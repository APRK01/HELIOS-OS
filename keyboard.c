#include "keyboard.h"
#include "uart.h"
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

static uint64_t kbd_base = 0;

static uint64_t to_phys(void *vaddr, uint64_t vbase, uint64_t pbase) {
  return (uint64_t)vaddr - vbase + pbase;
}

int keyboard_init(uint64_t hhdm_offset, uint64_t kernel_vbase,
                  uint64_t kernel_pbase) {
  extern uint64_t virtio_find_input_device(uint64_t hhdm_offset,
                                           int want_tablet);
  kbd_base = virtio_find_input_device(hhdm_offset, 0);
  if (!kbd_base) {
    uart_putc('E');
    uart_putc('1');
    uart_putc('\n');
    return 1;
  }

  uint32_t version = *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_VERSION);
  if (version != 1) {

    uart_putc('E');
    uart_putc('V');
    uart_putc('\n');
    return 1;
  }

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_STATUS) = 0;
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_STATUS) =
      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_GUEST_PAGE_SIZE) = PAGE_SIZE;

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_SEL) = 0;

  uint32_t max_num =
      *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_NUM_MAX);
  if (max_num < QUEUE_SIZE) {
    uart_putc('E');
    uart_putc('Q');
    uart_putc('\n');
    return 2;
  }
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_NUM) = QUEUE_SIZE;

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_ALIGN) = PAGE_SIZE;

  desc_ptr = (struct virtq_desc *)vq_buffer;
  avail_ptr = (struct vq_avail_fixed *)(vq_buffer +
                                        sizeof(struct virtq_desc) * QUEUE_SIZE);

  used_ptr = (struct vq_used_fixed *)(vq_buffer + PAGE_SIZE);

  uint64_t phys_base = to_phys(vq_buffer, kernel_vbase, kernel_pbase);
  uint32_t pfn = (uint32_t)(phys_base / PAGE_SIZE);

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_PFN) = pfn;

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

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_STATUS) =
      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
      VIRTIO_STATUS_DRIVER_OK;
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

  return 0;
}

static int ev_to_key(uint16_t code) {
  if (code == 28)
    return '\n';
  if (code == 1)
    return KEY_ESC;
  if (code == 14)
    return 8;
  if (code == 57)
    return ' ';
  if (code == 111)
    return KEY_DEL;

  if (code == 59)
    return KEY_F1;
  if (code == 60)
    return KEY_F2;

  if (code == 103)
    return KEY_UP;
  if (code == 108)
    return KEY_DOWN;
  if (code == 105)
    return KEY_LEFT;
  if (code == 106)
    return KEY_RIGHT;
  if (code == 102)
    return KEY_HOME;
  if (code == 107)
    return KEY_END;
  if (code == 104)
    return KEY_PGUP;
  if (code == 109)
    return KEY_PGDN;

  static const char row1[] = "1234567890-=";
  if (code >= 2 && code <= 13)
    return row1[code - 2];
  static const char row2[] = "qwertyuiop[]";
  if (code >= 16 && code <= 27)
    return row2[code - 16];
  static const char row3[] = "asdfghjkl;'`";
  if (code >= 30 && code <= 41)
    return row3[code - 30];
  static const char row4[] = "\\zxcvbnm,./";
  if (code >= 43 && code <= 53)
    return row4[code - 43];
  return 0;
}

int keyboard_has_char(void) {
  if (!kbd_base)
    return 0;

  __asm__ volatile("dc ivac, %0" ::"r"(&used_ptr->idx) : "memory");
  __asm__ volatile("dmb sy" ::: "memory");

  return (used_ptr->idx != last_used_idx);
}

int keyboard_getc(void) {
  while (!keyboard_has_char()) {
    __asm__ volatile("yield");
  }

  uint16_t head = last_used_idx % QUEUE_SIZE;

  __asm__ volatile("dc ivac, %0" ::"r"(&used_ptr->ring[head]) : "memory");
  __asm__ volatile("dmb sy" ::: "memory");

  struct virtq_used_elem *e = (struct virtq_used_elem *)&used_ptr->ring[head];
  struct virtio_input_event *evt = &events[e->id];

  __asm__ volatile("dc ivac, %0" ::"r"(evt) : "memory");
  __asm__ volatile("dmb sy" ::: "memory");

  int c = 0;
  if (evt->type == 1 && evt->value == 1) {
    c = ev_to_key(evt->code);
  }

  avail_ptr->ring[avail_ptr->idx % QUEUE_SIZE] = e->id;
  avail_ptr->idx++;

  __asm__ volatile(
      "dc civac, %0" ::"r"(&avail_ptr->ring[avail_ptr->idx % QUEUE_SIZE])
      : "memory");
  __asm__ volatile("dc civac, %0" ::"r"(&avail_ptr->idx) : "memory");
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

  last_used_idx++;

  return c;
}
