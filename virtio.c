#include "virtio.h"
#include <stdint.h>

static inline uint32_t virtio_read32(uint64_t base, uint32_t offset) {
  return *(volatile uint32_t *)(base + offset);
}

static inline void virtio_write32(uint64_t base, uint32_t offset,
                                  uint32_t val) {
  *(volatile uint32_t *)(base + offset) = val;
  __asm__ volatile("dmb sy" ::: "memory");
}

static inline void virtio_write8(uint64_t base, uint32_t offset, uint8_t val) {
  *(volatile uint8_t *)(base + offset) = val;
  __asm__ volatile("dmb sy" ::: "memory");
}

uint64_t virtio_find_device(uint32_t device_id, uint64_t hhdm_offset) {
  for (uint64_t i = 0; i < 32; i++) {
    uint64_t base = 0x0a000000 + (i * 0x200) + hhdm_offset;

    uint32_t magic = virtio_read32(base, VIRTIO_MMIO_MAGIC_VALUE);
    uint32_t dev_id = virtio_read32(base, VIRTIO_MMIO_DEVICE_ID);

    if (magic == 0x74726976 && dev_id == device_id) {
      return base;
    }
  }
  return 0;
}

#define VIRTIO_INPUT_CFG_EV_BITS 0x11
#define EV_KEY 1
#define EV_ABS 3

static int virtio_input_supports_event(uint64_t base, uint8_t ev_type) {
  volatile uint8_t *cfg = (volatile uint8_t *)(base + VIRTIO_MMIO_CONFIG);
  cfg[0] = VIRTIO_INPUT_CFG_EV_BITS;
  cfg[1] = ev_type;
  __asm__ volatile("dmb sy" ::: "memory");
  uint8_t size = cfg[2];
  return size > 0;
}

uint64_t virtio_find_input_device(uint64_t hhdm_offset, int want_tablet) {
  for (uint64_t i = 0; i < 32; i++) {
    uint64_t base = 0x0a000000 + (i * 0x200) + hhdm_offset;
    uint32_t magic = virtio_read32(base, VIRTIO_MMIO_MAGIC_VALUE);
    uint32_t dev_id = virtio_read32(base, VIRTIO_MMIO_DEVICE_ID);
    if (magic == 0x74726976 && dev_id == VIRTIO_ID_INPUT) {
      int has_abs = virtio_input_supports_event(base, EV_ABS);
      int has_key = virtio_input_supports_event(base, EV_KEY);
      if (want_tablet && has_abs) {
        return base;
      }
      if (!want_tablet && has_key && !has_abs) {
        return base;
      }
    }
  }
  return 0;
}
