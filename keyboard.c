#include "keyboard.h"
#include "uart.h"
#include "virtio.h"
#include <stddef.h>
#include <stdint.h>

// VirtIO Input Event Structure
struct virtio_input_event {
  uint16_t type;
  uint16_t code;
  uint32_t value;
};

// Queue configuration
#define QUEUE_SIZE 8
#define PAGE_SIZE 4096

// Legacy MMIO Register Offsets (V1)
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028
#define VIRTIO_MMIO_QUEUE_PFN 0x040
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03c

// Fixed-size virtqueue structures
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

// Contiguous buffer for Legacy V1 Layout
// We need enough space for Desc + Avail + Padding + Used.
// With 4096 byte alignment/page size:
// Desc (128 bytes) + Avail (22 bytes) -> Page 1
// Used (70 bytes) -> Page 2 (Must be aligned to QueueAlign)
// Total 2 pages = 8192 bytes.
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
  extern uint64_t virtio_find_device(uint32_t device_id, uint64_t hhdm_offset);
  kbd_base = virtio_find_device(VIRTIO_ID_INPUT, hhdm_offset);
  if (!kbd_base) {
    uart_putc('E');
    uart_putc('1');
    uart_putc('\n');
    return 1;
  }

  uint32_t version = *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_VERSION);
  if (version != 1) {
    // We only support Legacy V1 for now since that's what QEMU provides
    uart_putc('E');
    uart_putc('V');
    uart_putc('\n');
    return 1;
  }

  // Legacy Initialization Sequence
  // 1. Reset
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_STATUS) = 0;
  __asm__ volatile("dmb sy" ::: "memory");

  // 2. Acknowledge + Driver
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_STATUS) =
      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
  __asm__ volatile("dmb sy" ::: "memory");

  // 3. Page Size negotiation
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_GUEST_PAGE_SIZE) = PAGE_SIZE;

  // 4. Select Queue 0
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_SEL) = 0;

  // Check max size
  uint32_t max_num =
      *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_NUM_MAX);
  if (max_num < QUEUE_SIZE) {
    uart_putc('E');
    uart_putc('Q');
    uart_putc('\n');
    return 2;
  }
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_NUM) = QUEUE_SIZE;

  // 5. Setup Alignment
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_ALIGN) = PAGE_SIZE;

  // 6. Calculate Physical PFN
  // Map our contiguous buffer pointers
  desc_ptr = (struct virtq_desc *)vq_buffer;
  avail_ptr = (struct vq_avail_fixed *)(vq_buffer +
                                        sizeof(struct virtq_desc) * QUEUE_SIZE);
  // Used ring starts at the next page boundary because of 4096 alignment
  used_ptr = (struct vq_used_fixed *)(vq_buffer + PAGE_SIZE);

  uint64_t phys_base = to_phys(vq_buffer, kernel_vbase, kernel_pbase);
  uint32_t pfn = (uint32_t)(phys_base / PAGE_SIZE);

  // Write PFN to device
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_PFN) = pfn;

  // 7. Setup Descriptors
  for (int i = 0; i < QUEUE_SIZE; i++) {
    uint64_t event_phys = to_phys(&events[i], kernel_vbase, kernel_pbase);
    desc_ptr[i].addr = event_phys;
    desc_ptr[i].len = sizeof(struct virtio_input_event);
    desc_ptr[i].flags = VIRTQ_DESC_F_WRITE;
    desc_ptr[i].next = 0;
    avail_ptr->ring[i] = i;
  }
  avail_ptr->idx = QUEUE_SIZE;

  // Flush cache for the whole buffer
  // We flush the range of desc + avail (Page 1) and Used (Page 2)
  for (int i = 0; i < PAGE_SIZE * 2; i += 64) {
    __asm__ volatile("dc civac, %0" ::"r"(vq_buffer + i) : "memory");
  }
  __asm__ volatile("dmb sy" ::: "memory");

  // 8. Driver OK
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_STATUS) =
      VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
      VIRTIO_STATUS_DRIVER_OK;
  __asm__ volatile("dmb sy" ::: "memory");

  // Notify
  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

  return 0;
}

static char ev_to_ascii(uint16_t code) {
  if (code == 28)
    return '\n';
  if (code == 1)
    return 27;
  if (code == 14)
    return 8;
  if (code == 57)
    return ' ';

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

  // Invalidate cache for used idx
  __asm__ volatile("dc ivac, %0" ::"r"(&used_ptr->idx) : "memory");
  __asm__ volatile("dmb sy" ::: "memory");

  return (used_ptr->idx != last_used_idx);
}

char keyboard_getc(void) {
  while (!keyboard_has_char()) {
    __asm__ volatile("yield");
  }

  uint16_t head = last_used_idx % QUEUE_SIZE;

  // Invalidate cache for the used ring element
  __asm__ volatile("dc ivac, %0" ::"r"(&used_ptr->ring[head]) : "memory");
  __asm__ volatile("dmb sy" ::: "memory");

  struct virtq_used_elem *e = (struct virtq_used_elem *)&used_ptr->ring[head];
  struct virtio_input_event *evt = &events[e->id];

  // Invalidate cache for event data
  __asm__ volatile("dc ivac, %0" ::"r"(evt) : "memory");
  __asm__ volatile("dmb sy" ::: "memory");

  char c = 0;
  if (evt->type == 1 && evt->value == 1) {
    c = ev_to_ascii(evt->code);
  }

  // Recycle
  avail_ptr->ring[avail_ptr->idx % QUEUE_SIZE] = e->id;
  avail_ptr->idx++;

  // Flush update to avail ring
  __asm__ volatile(
      "dc civac, %0" ::"r"(&avail_ptr->ring[avail_ptr->idx % QUEUE_SIZE])
      : "memory");
  __asm__ volatile("dc civac, %0" ::"r"(&avail_ptr->idx) : "memory");
  __asm__ volatile("dmb sy" ::: "memory");

  *(volatile uint32_t *)(kbd_base + VIRTIO_MMIO_QUEUE_NOTIFY) = 0;

  last_used_idx++;
  if (c == 0)
    return keyboard_getc();
  return c;
}
