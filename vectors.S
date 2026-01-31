.section .text.vectors

.global vectors
.balign 2048
vectors:
     
     
     
     
    .balign 128
    b       sync_invalid_el1t
     
    .balign 128
    b       irq_invalid_el1t
     
    .balign 128
    b       fiq_invalid_el1t
     
    .balign 128
    b       serror_invalid_el1t

     
     
     
     
    .balign 128
    b       sync_el1h
     
    .balign 128
    b       irq_el1h
     
    .balign 128
    b       fiq_invalid_el1h
     
    .balign 128
    b       serror_invalid_el1h

     
     
     
    .balign 128
    b       sync_invalid_el064
    .balign 128
    b       irq_invalid_el064
    .balign 128
    b       fiq_invalid_el064
    .balign 128
    b       serror_invalid_el064

     
     
     
    .balign 128
    b       sync_invalid_el032
    .balign 128
    b       irq_invalid_el032
    .balign 128
    b       fiq_invalid_el032
    .balign 128
    b       serror_invalid_el032

 
 
 
.macro INVALID_HANDLER name
\name:
     
    wfi
    b       \name
.endm

INVALID_HANDLER sync_invalid_el1t
INVALID_HANDLER irq_invalid_el1t
INVALID_HANDLER fiq_invalid_el1t
INVALID_HANDLER serror_invalid_el1t

INVALID_HANDLER fiq_invalid_el1h
INVALID_HANDLER serror_invalid_el1h

INVALID_HANDLER sync_invalid_el064
INVALID_HANDLER irq_invalid_el064
INVALID_HANDLER fiq_invalid_el064
INVALID_HANDLER serror_invalid_el064

INVALID_HANDLER sync_invalid_el032
INVALID_HANDLER irq_invalid_el032
INVALID_HANDLER fiq_invalid_el032
INVALID_HANDLER serror_invalid_el032

 
 
 

 
sync_el1h:
     
    eret

 
irq_el1h:
     
     
     
     
    sub     sp, sp, #192
    
     
    stp     x0, x1, [sp, #0]
    stp     x2, x3, [sp, #16]
    stp     x4, x5, [sp, #32]
    stp     x6, x7, [sp, #48]
    stp     x8, x9, [sp, #64]
    stp     x10, x11, [sp, #80]
    stp     x12, x13, [sp, #96]
    stp     x14, x15, [sp, #112]
    stp     x16, x17, [sp, #128]
     
    stp     x18, x29, [sp, #144]
    str     x30, [sp, #160]

     
    mrs     x0, elr_el1
    mrs     x1, spsr_el1
    stp     x0, x1, [sp, #176]

     
    bl      irq_handler

     
    ldp     x2, x3, [sp, #176]
    msr     elr_el1, x2
    msr     spsr_el1, x3

     
    ldr     x30, [sp, #160]
    ldp     x18, x29, [sp, #144]
    ldp     x16, x17, [sp, #128]
    ldp     x14, x15, [sp, #112]
    ldp     x12, x13, [sp, #96]
    ldp     x10, x11, [sp, #80]
    ldp     x8, x9, [sp, #64]
    ldp     x6, x7, [sp, #48]
    ldp     x4, x5, [sp, #32]
    ldp     x2, x3, [sp, #16]
    ldp     x0, x1, [sp, #0]
    add     sp, sp, #192

    eret
