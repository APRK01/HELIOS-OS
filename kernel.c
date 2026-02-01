#include "console.h"
#include "gic.h"
#include "gui.h"
#include "heap.h"
#include "keyboard.h"
#include "limine.h"
#include "mouse.h"
#include "pmm.h"
#include "process.h"
#include "shell.h"
#include "timer.h"
#include "tmpfs.h"
#include "uart.h"
#include "vfs.h"
#include <stddef.h>
#include <stdint.h>

__attribute__((
    used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(0);

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_hhdm_request
    hhdm_request = {.id = LIMINE_HHDM_REQUEST, .revision = 0};

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests"))) static volatile struct
    limine_kernel_address_request kernel_address_request = {
        .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0};

__attribute__((used,
               section(".limine_requests_"
                       "start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used,
    section(
        ".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

static void hcf(void) {
  for (;;) {
    __asm__ volatile("wfi");
  }
}

void _start(void) {

  uint64_t uart_vbase = UART0_PHYS;
  if (hhdm_request.response != NULL) {
    uart_vbase += hhdm_request.response->offset;
  }
  uart_init(uart_vbase);

  if (framebuffer_request.response != NULL &&
      framebuffer_request.response->framebuffer_count > 0) {
    struct limine_framebuffer *fb =
        framebuffer_request.response->framebuffers[0];
    console_init(fb);
  }

  if (hhdm_request.response != NULL &&
      kernel_address_request.response != NULL) {
    uint64_t hhdm = hhdm_request.response->offset;
    uint64_t vbase = kernel_address_request.response->virtual_base;
    uint64_t pbase = kernel_address_request.response->physical_base;
    keyboard_init(hhdm, vbase, pbase);
    mouse_init(hhdm, vbase, pbase);
    gui_set_hhdm(hhdm);
  }

  if (memmap_request.response != NULL && hhdm_request.response != NULL) {
    pmm_init(memmap_request.response, hhdm_request.response->offset);
  } else {
    console_print("KERNEL PANIC: No Memory Map or HHDM!\n");
    hcf();
  }

  heap_init();

  process_init();

  fs_root = tmpfs_init();
  if (fs_root) {
    console_print("VFS: TmpFS mounted at /\n");
  } else {
    console_print("VFS: Failed to mount TmpFS!\n");
  }

  shell_run();

  hcf();
}
