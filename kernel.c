#include "console.h"
#include "gic.h"
#include "heap.h"
#include "keyboard.h"
#include "limine.h"
#include "pmm.h"
#include "process.h"
#include "shell.h"
#include "timer.h"
#include "tmpfs.h"
#include "uart.h"
#include "vfs.h"
#include <stddef.h>
#include <stdint.h>

// Request the Limine bootloader to set up a framebuffer
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

// Request markers
__attribute__((used,
               section(".limine_requests_"
                       "start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used,
    section(
        ".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

// Halt the CPU
static void hcf(void) {
  for (;;) {
    __asm__ volatile("wfi");
  }
}

// Kernel entry point
void _start(void) {
  // 1. Initialize UART via HHDM offset if available
  uint64_t uart_vbase = UART0_PHYS;
  if (hhdm_request.response != NULL) {
    uart_vbase += hhdm_request.response->offset;
  }
  uart_init(uart_vbase);

  // 2. Initialize console
  if (framebuffer_request.response != NULL &&
      framebuffer_request.response->framebuffer_count > 0) {
    struct limine_framebuffer *fb =
        framebuffer_request.response->framebuffers[0];
    console_init(fb);
  }

  // 3. Initialize keyboard with correct address translation info
  if (hhdm_request.response != NULL &&
      kernel_address_request.response != NULL) {
    uint64_t hhdm = hhdm_request.response->offset;
    uint64_t vbase = kernel_address_request.response->virtual_base;
    uint64_t pbase = kernel_address_request.response->physical_base;
    keyboard_init(hhdm, vbase, pbase);
  }

  // 4. Initialize PMM
  if (memmap_request.response != NULL && hhdm_request.response != NULL) {
    pmm_init(memmap_request.response, hhdm_request.response->offset);
  } else {
    console_print("KERNEL PANIC: No Memory Map or HHDM!\n");
    hcf();
  }

  // 5. Initialize Heap
  heap_init();

  // 5.5 Initialize Multitasking
  process_init();

  // 5.6 Interrupts DISABLED - conflicts with VirtIO keyboard
  // Investigation needed: GIC/Timer IRQ breaks VirtIO MMIO polling
  // extern char vectors[];
  // __asm__ volatile("msr vbar_el1, %0" ::"r"(vectors));
  // gic_init();
  // timer_init(10);
  // __asm__ volatile("msr daifclr, #2");

  // 6. Initialize Filesystem
  fs_root = tmpfs_init();
  if (fs_root) {
    console_print("VFS: TmpFS mounted at /\n");
  } else {
    console_print("VFS: Failed to mount TmpFS!\n");
  }

  // 7. Launch the shell
  shell_run();

  // Loop forever
  hcf();
}
