 
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)

 
ENTRY(_start)

 
 
KERNEL_OFFSET = 0xffffffff80000000;

PHDRS {
    text    PT_LOAD FLAGS(5);    
    rodata  PT_LOAD FLAGS(4);    
    data    PT_LOAD FLAGS(6);    
}

SECTIONS {
     
    . = KERNEL_OFFSET;

     
    .limine_requests_start : {
        KEEP(*(.limine_requests_start))
    } :text

    .limine_requests : {
        KEEP(*(.limine_requests))
    } :text

    .limine_requests_end : {
        KEEP(*(.limine_requests_end))
    } :text

     
    .text : {
        *(.text .text.*)
    } :text

     
    . = ALIGN(4096);

     
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

     
    . = ALIGN(4096);

     
    .data : {
        *(.data .data.*)
    } :data

     
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

     
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
        *(.comment)
    }
}
