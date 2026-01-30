.section .text.vectors

.global vectors
.balign 2048
vectors:
    // ============================================
    // Current EL with SP0 (not used in kernel)
    // ============================================
    // Synchronous
    .balign 128
    b       sync_invalid_el1t
    // IRQ
    .balign 128
    b       irq_invalid_el1t
    // FIQ
    .balign 128
    b       fiq_invalid_el1t
    // SError
    .balign 128
    b       serror_invalid_el1t

    // ============================================
    // Current EL with SPx (This is where kernel runs)
    // ============================================
    // Synchronous
    .balign 128
    b       sync_el1h
    // IRQ
    .balign 128
    b       irq_el1h
    // FIQ
    .balign 128
    b       fiq_invalid_el1h
    // SError
    .balign 128
    b       serror_invalid_el1h

    // ============================================
    // Lower EL using AArch64 (Userspace - not used yet)
    // ============================================
    .balign 128
    b       sync_invalid_el064
    .balign 128
    b       irq_invalid_el064
    .balign 128
    b       fiq_invalid_el064
    .balign 128
    b       serror_invalid_el064

    // ============================================
    // Lower EL using AArch32 (Not used)
    // ============================================
    .balign 128
    b       sync_invalid_el032
    .balign 128
    b       irq_invalid_el032
    .balign 128
    b       fiq_invalid_el032
    .balign 128
    b       serror_invalid_el032

// ============================================
// Invalid Exception Handlers (Panic)
// ============================================
.macro INVALID_HANDLER name
\name:
    // For now, just halt
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

// ============================================
// Valid Handlers
// ============================================

// Synchronous exception from EL1 (e.g., SVC, page fault)
sync_el1h:
    // Not handling sync exceptions now, just return
    eret

// IRQ from EL1 - The important one!
irq_el1h:
    // Save ALL caller-saved registers (x0-x18) and frame pointer/link register
    // We need to save 21 registers (x0-x18, x29, x30) = 168 bytes
    // Plus ELR and SPSR = 16 bytes
    // Total = 184 bytes, round up to 192 for 16-byte alignment
    sub     sp, sp, #192
    
    // Save x0-x17 (9 pairs = 144 bytes)
    stp     x0, x1, [sp, #0]
    stp     x2, x3, [sp, #16]
    stp     x4, x5, [sp, #32]
    stp     x6, x7, [sp, #48]
    stp     x8, x9, [sp, #64]
    stp     x10, x11, [sp, #80]
    stp     x12, x13, [sp, #96]
    stp     x14, x15, [sp, #112]
    stp     x16, x17, [sp, #128]
    // Save x18, x29, x30 (need 3 registers - save x18+x29 as pair, x30 separate)
    stp     x18, x29, [sp, #144]
    str     x30, [sp, #160]

    // Save exception state (ELR and SPSR)
    mrs     x0, elr_el1
    mrs     x1, spsr_el1
    stp     x0, x1, [sp, #176]

    // Call C handler
    bl      irq_handler

    // Restore exception state FIRST (use x2, x3 to avoid clobbering)
    ldp     x2, x3, [sp, #176]
    msr     elr_el1, x2
    msr     spsr_el1, x3

    // Restore all registers
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
