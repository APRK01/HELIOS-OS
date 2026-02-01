#include "mouse.h"
#include "virtio.h"
#include <stddef.h>
#include <stdint.h>

struct virtio_input_event {
  uint16_t type;
  uint16_t code;
  uint32_t value;
};

#define QUEUE_SIZE 8
#define PAGE_SIZE 4096

#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028
#define VIRTIO_MMIO_QUEUE_PFN 0x040
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03c

struct vq_avail_fixed {
  uint16_t flags;
  uint16_t idx;
  uint16_t ring[QUEUE_SIZE];
} __attribute__((packed));

struct vq_used_fixed {
  uint16_t flags;
  uint16_t idx;
  struct virtq_used_elem ring[QUEUE_SIZE];
} __attribute__((packed));

static uint8_t vq_buffer[PAGE_SIZE * 2] __attribute__((aligned(PAGE_SIZE)));
static struct virtio_input_event events[QUEUE_SIZE];
static uint16_t last_used_idx = 0;

static volatile struct virtq_desc *desc_ptr;
static volatile struct vq_avail_fixed *avail_ptr;
static volatile struct vq_used_fixed *used_ptr;

static uint64_t mouse_base = 0;

static int mouse_x = 0;
static int mouse_y = 0;
static int mouse_buttons = 0;
static int screen_width = 1024;
static int screen_height = 768;

static uint64_t to_phys(void *vaddr, uint64_t vbase, uint64_t pbase) {
  return (uint64_t)vaddr - vbase + pbase;
}

uint64_t virtio_find_device_nth(uint32_t device_id, uint64_t hhdm_offset,
                                int n) {
  int found = 0;
  for (uint64_t i = 0; i < 32; i++) {
    uint64_t base = 0x0a000000 + (i * 0x200) + hhdm_offset;
    uint32_t magic = *(volatile uint32_t *)(base + VIRTIO_MMIO_MAGIC_VALUE);
    uint32_t dev_id = *(volatile uint32_t *)(base + VIRTIO_MMIO_DEVICE_ID);
    if (magic == 0x74726976 && dev_id == device_id) {
      if (found == n)
        return base;
      found++;
    }
  }
  return 0;
}

int mouse_init(uint64_t hhdm_offset, uint64_t kernel_vbase,
               uint64_t kernel_pbase) {
  extern uint64_t virtio_find_input_device(uint64_t hhdm_offset,
                                           int want_tablet);
  mouse_base = virtio_find_input_device(hhdm_offset, 1);
  if (!mouse_base) {
    return 1;
  }

  uint32_t version = *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_VERSION);
  if (version != 1) {
    return 1;
  }

  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_STATUS) = 0;
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_STATUS) =
      VIRTIO_STATUS_ACKNOWLEDGE;
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_STATUS) =
      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_GUEST_PAGE_SIZE) = PAGE_SIZE;

  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_QUEUE_SEL) = 0;
  uint32_t max_q =
      *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_QUEUE_NUM_MAX);
  if (max_q == 0 || max_q < QUEUE_SIZE) {
    return 2;
  }
  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_QUEUE_NUM) = QUEUE_SIZE;
  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_QUEUE_ALIGN) = PAGE_SIZE;

  desc_ptr = (struct virtq_desc *)vq_buffer;
  avail_ptr = (struct vq_avail_fixed *)(vq_buffer +
                                        sizeof(struct virtq_desc) * QUEUE_SIZE);
  used_ptr = (struct vq_used_fixed *)(vq_buffer + PAGE_SIZE);

  uint64_t phys_base = to_phys(vq_buffer, kernel_vbase, kernel_pbase);
  uint32_t pfn = (uint32_t)(phys_base / PAGE_SIZE);

  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_QUEUE_PFN) = pfn;

  for (int i = 0; i < QUEUE_SIZE; i++) {
    uint64_t event_phys = to_phys(&events[i], kernel_vbase, kernel_pbase);
    desc_ptr[i].addr = event_phys;
    desc_ptr[i].len = sizeof(struct virtio_input_event);
    desc_ptr[i].flags = VIRTQ_DESC_F_WRITE;
    desc_ptr[i].next = 0;
    avail_ptr->ring[i] = i;
  }
  avail_ptr->idx = QUEUE_SIZE;

  for (int i = 0; i < PAGE_SIZE * 2; i += 64) {
    __asm__ volatile("dc civac, %0" ::"r"(vq_buffer + i) : "memory");
  }
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_STATUS) =
      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
      VIRTIO_STATUS_DRIVER_OK;
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

  mouse_x = screen_width / 2;
  mouse_y = screen_height / 2;

  return 0;
}

void mouse_poll(void) {
  if (!mouse_base)
    return;

  __asm__ volatile("dc ivac, %0" ::"r"(&used_ptr->idx) : "memory");
  __asm__ volatile("dmb sy" ::: "memory");

  while (used_ptr->idx != last_used_idx) {
    uint16_t head = last_used_idx % QUEUE_SIZE;

    __asm__ volatile("dc ivac, %0" ::"r"(&used_ptr->ring[head]) : "memory");
    __asm__ volatile("dmb sy" ::: "memory");

    struct virtq_used_elem *e = (struct virtq_used_elem *)&used_ptr->ring[head];
    struct virtio_input_event *evt = &events[e->id];

    __asm__ volatile("dc ivac, %0" ::"r"(evt) : "memory");
    __asm__ volatile("dmb sy" ::: "memory");

    if (evt->type == 3) {
      if (evt->code == 0) {
        mouse_x = (evt->value * screen_width) / 32768;
      } else if (evt->code == 1) {
        mouse_y = (evt->value * screen_height) / 32768;
      }
    } else if (evt->type == 1) {
      if (evt->code == 0x110) {
        if (evt->value)
          mouse_buttons |= MOUSE_BTN_LEFT;
        else
          mouse_buttons &= ~MOUSE_BTN_LEFT;
      } else if (evt->code == 0x111) {
        if (evt->value)
          mouse_buttons |= MOUSE_BTN_RIGHT;
        else
          mouse_buttons &= ~MOUSE_BTN_RIGHT;
      } else if (evt->code == 0x112) {
        if (evt->value)
          mouse_buttons |= MOUSE_BTN_MIDDLE;
        else
          mouse_buttons &= ~MOUSE_BTN_MIDDLE;
      }
    }

    avail_ptr->ring[avail_ptr->idx % QUEUE_SIZE] = e->id;
    avail_ptr->idx++;

    __asm__ volatile(
        "dc civac, %0" ::"r"(&avail_ptr->ring[avail_ptr->idx % QUEUE_SIZE])
        : "memory");
    __asm__ volatile("dc civac, %0" ::"r"(&avail_ptr->idx) : "memory");
    __asm__ volatile("dmb sy" ::: "memory");

    *(volatile uint32_t *)(mouse_base + VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

    last_used_idx++;
  }
}

int mouse_get_x(void) { return mouse_x; }
int mouse_get_y(void) { return mouse_y; }
int mouse_get_buttons(void) { return mouse_buttons; }
