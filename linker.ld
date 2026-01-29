/* Tell the linker that the output format is ELF ARM64 */
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)

/* Tell the linker that the kernel entry point is _start */
ENTRY(_start)

/* Define the virtual address where the kernel will be loaded */
/* Limine loads the kernel at the higher half by default */
KERNEL_OFFSET = 0xffffffff80000000;

PHDRS {
    text    PT_LOAD FLAGS(5);   /* Execute + Read */
    rodata  PT_LOAD FLAGS(4);   /* Read only */
    data    PT_LOAD FLAGS(6);   /* Read + Write */
}

SECTIONS {
    /* Start at higher half virtual address */
    . = KERNEL_OFFSET;

    /* Limine requests section - must come first */
    .limine_requests_start : {
        KEEP(*(.limine_requests_start))
    } :text

    .limine_requests : {
        KEEP(*(.limine_requests))
    } :text

    .limine_requests_end : {
        KEEP(*(.limine_requests_end))
    } :text

    /* Text section (executable code) */
    .text : {
        *(.text .text.*)
    } :text

    /* Align to page boundary */
    . = ALIGN(4096);

    /* Read-only data */
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    /* Align to page boundary */
    . = ALIGN(4096);

    /* Initialized data */
    .data : {
        *(.data .data.*)
    } :data

    /* Uninitialized data (BSS) */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
        *(.comment)
    }
}
