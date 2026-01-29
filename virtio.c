#include "virtio.h"
#include <stdint.h>

// For simplicity, we'll use a single global virtio configuration
// and search in the standard QEMU virtio-mmio range (0x0a000000)

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

// Find a VirtIO device of a specific type
uint64_t virtio_find_device(uint32_t device_id, uint64_t hhdm_offset) {
  // Search the first 32 possible MMIO slots (each is 0x200 bytes)
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
