// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 feiyehua
/*
 * sep-artk-derive.S
 *
 * SEP-side, bare-metal A32 payload for the t7000 (AppleSEPROM-175.0.0.1.9)
 * post-handoff execution context:
 *
 *   - SVC mode, A32, IRQ/FIQ masked
 *   - SCTLR.M may be 0 (ROM page tables still in TTBR0/TTBCR)
 *   - low VA space [0, M-S) is identity mapped and RWX
 *   - SoC window [0x30000000, 0x40000000) is mapped to 0x2_00000000
 *   - the ROM window [0x10000000, 0x10100000) is NOT usable, so this code
 *     never calls into SEPROM
 *
 * It implements the EEPROM backend-1 derivation path from
 * research/a8/SEPROM-K-ART-DERIVATION.md:
 *
 *   1. initialise the I2C controller and GPIO 4/5
 *   2. read the 16-byte EEPROM header (seed8 || block ids || tag6)
 *   3. build the 24-byte KDF context: zero16 || seed8
 *   4. run crypto command 0x204 (516) with domain word "ARTM" -> 32-byte K_ART
 *   5. store K_ART at VA 0x10140000
 *
 * The payload is written for the sep-pwn-smoke loader: the "branch" opcode
 * reaches _start with BLX, so LR points back into the smoke message loop.
 * On completion the payload returns to that loop and reports its status over
 * the mailbox (opcode 0xAD = ok, 0xAE = I2C failure, 0xAF = KDF failure).
 *
 * Fixed scratch layout in the low RWX window:
 *
 *   0x00004000  stack top (grows down)
 *   0x00008000  80-byte result buffer:
 *               +0x00  header[16]
 *               +0x10  KDF input[32]
 *               +0x30  K_ART[32]
 *
 * Nothing here has to be linked at a particular address: only the scratch
 * addresses above and the MMIO constants are absolute.
 */

.syntax unified
.arch armv7-a
.arm
.text
.balign 4
.global _start

/* Low-window scratch */
.equ STACK_TOP,       0x00004000
.equ RESULT_BUF,      0x00008000
.equ RESULT_SIZE,     0x50

/* SoC window (VA -> 0x2_00000000) */
.equ I2C_CMD,         0x3D700000    /* +0x00 command/data */
.equ I2C_STS,         0x3D700004    /* +0x04 status */
.equ I2C_CTRL,        0x3D700014    /* +0x14 control/error */
.equ I2C_CFG,         0x3D70001C    /* +0x1C clock/irq config */
.equ PINMUX_BASE,     0x3DF00000
.equ CRYPTO_BASE,     0x3D300000
.equ MAILBOX_BASE,    0x3DA00BA0

/* Deliverable */
.equ K_ART_VA,        0x10140000

.macro MMIO_REG reg, addr
    movw    \reg, #:lower16:\addr
    movt    \reg, #:upper16:\addr
.endm

/* ------------------------------------------------------------------ */
/* Entry                                                               */
/* ------------------------------------------------------------------ */
_start:
    /* Save the smoke loader's stack pointer and callee-saved registers. */
    mov     r0, sp
    movw    sp, #:lower16:STACK_TOP
    movt    sp, #:upper16:STACK_TOP
    push    {r0, r4-r7, lr}

    /*
     * The handoff stub leaves MMU off and the ROM runtime page tables in
     * TTBR0.  The smoke loader normally turns the MMU on already; only do
     * it ourselves if it is still off, so a working mapping is untouched.
     */
    mrc     p15, 0, r0, c1, c0, 0
    tst     r0, #1
    bne     .Lmmu_on

    dsb     sy
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0   @ TLBIALL
    dsb     sy
    isb     sy
    mrc     p15, 0, r0, c1, c0, 0
    orr     r0, r0, #1              @ SCTLR.M
    mcr     p15, 0, r0, c1, c0, 0
    isb     sy

.Lmmu_on:
    /* Clear header + kdf input + key. */
    movw    r4, #:lower16:RESULT_BUF
    movt    r4, #:upper16:RESULT_BUF
    mov     r1, #0
    mov     r2, #0
1:  str     r1, [r4, r2]
    add     r2, r2, #4
    cmp     r2, #RESULT_SIZE
    bne     1b

    /* Bring up I2C the same way SEPROM does for backend 1. */
    bl      i2c_init
    dsb     sy

    /* Read EEPROM header at offset 0, 16 bytes. */
    movw    r0, #:lower16:RESULT_BUF
    movt    r0, #:upper16:RESULT_BUF
    bl      eeprom_read_header16
    cmp     r0, #0
    bne     .Lerr_i2c

    /* --------------------------------------------------------------
     * Build the 32-byte crypto input:
     *   0x4152544D "MTRA" | 0 | zero16 | seed8
     * (SEPROM stores the domain label reversed as 4D 54 52 41.)
     * -------------------------------------------------------------- */
    movw    r4, #:lower16:RESULT_BUF
    movt    r4, #:upper16:RESULT_BUF
    movw    r0, #0x544D
    movt    r0, #0x4152             @ 0x4152544D
    str     r0, [r4, #0x10]
    mov     r0, #0
    str     r0, [r4, #0x14]         @ parameter = 0
    str     r0, [r4, #0x18]         @ zero16
    str     r0, [r4, #0x1C]
    str     r0, [r4, #0x20]
    str     r0, [r4, #0x24]
    ldr     r0, [r4, #0x00]         @ header[0:4] = seed[0:4]
    ldr     r1, [r4, #0x04]         @ header[4:8] = seed[4:8]
    str     r0, [r4, #0x28]
    str     r1, [r4, #0x2C]

    /* --------------------------------------------------------------
     * Write the 8 input words and run command 0x204 on the crypto
     * engine.  Register offsets follow SEPROM's command516 path.
     * -------------------------------------------------------------- */
    MMIO_REG r5, CRYPTO_BASE
    add     r0, r4, #0x10
    mov     r1, #0
1:  ldr     r2, [r0, r1]
    add     r3, r5, r1
    str     r2, [r3, #0x40]
    add     r1, r1, #4
    cmp     r1, #32
    bne     1b

    movw    r0, #0x0204
    str     r0, [r5, #0x08]         @ command 516
    mov     r0, #1
    str     r0, [r5, #0x04]         @ start
    dsb     sy

    /* Poll status: bit0 = done, bit1 = error.  Bounded, no WFE. */
    movw    r3, #0xFFFF
    movt    r3, #0x00FF             @ ~16M polls
2:  ldr     r0, [r5, #0x0C]
    tst     r0, #3
    bne     3f
    subs    r3, r3, #1
    bne     2b
    b       .Lerr_kdf
3:  tst     r0, #2
    bne     .Lerr_kdf

    /* Read 32-byte result into RESULT_BUF+0x30. */
    add     r1, r4, #0x30
    mov     r0, #0
1:  add     r2, r5, r0
    ldr     r3, [r2, #0x60]
    str     r3, [r1, r0]
    add     r0, r0, #4
    cmp     r0, #32
    bne     1b

    /* Deliver K_ART to VA 0x10140000. */
    MMIO_REG r2, K_ART_VA
    add     r1, r4, #0x30
    mov     r0, #0
1:  ldr     r3, [r1, r0]
    str     r3, [r2, r0]
    add     r0, r0, #4
    cmp     r0, #32
    bne     1b
    dsb     sy

    mov     r0, #0
    bl      report_status
    b       .Lreturn

.Lerr_i2c:
    mov     r0, #1
    b       .Lreport_error
.Lerr_kdf:
    mov     r0, #2
.Lreport_error:
    bl      report_status

.Lreturn:
    pop     {r0, r4-r7, lr}
    mov     sp, r0
    bx      lr

/* ------------------------------------------------------------------ */
/* I2C controller / pinmux                                             */
/* ------------------------------------------------------------------ */

/*
 * i2c_init
 *
 * Mirrors SEPROM's sep_i2c_initialize_with_clock + sep_i2c_initialize:
 *   configure_clock_divider(2500) -> writes 0x104 to 0x3D70001C
 *   initialize(); I2C_CTRL=0xFFFFFFFF; I2C+0x18=0; initialize()
 */
i2c_init:
    push    {r4, lr}

    MMIO_REG r0, I2C_CFG
    movw    r1, #0x0104
    str     r1, [r0]

    bl      i2c_init_body

    MMIO_REG r0, I2C_CTRL
    mvn     r1, #0
    str     r1, [r0]
    MMIO_REG r0, 0x3D700018
    mov     r1, #0
    str     r1, [r0]

    bl      i2c_init_body

    pop     {r4, pc}

/*
 * i2c_init_body
 *
 * GPIO 4 = SCL, GPIO 5 = SDA on t7000 backend 1.
 *   pinmux(4, 1)
 *   19 x { pinmux(5, i & 1); delay(50 ticks) }
 *   pinmux(4, 4); pinmux(5, 4)
 *   I2C_CFG |= 0x600
 */
i2c_init_body:
    push    {r4, lr}

    mov     r0, #4
    mov     r1, #1
    bl      pinmux_config

    mov     r4, #0
1:  mov     r0, #5
    and     r1, r4, #1
    bl      pinmux_config
    mov     r0, #0x32
    bl      delay_ticks
    add     r4, r4, #1
    cmp     r4, #0x13
    bne     1b

    mov     r0, #4
    mov     r1, #4
    bl      pinmux_config
    mov     r0, #5
    mov     r1, #4
    bl      pinmux_config

    MMIO_REG r0, I2C_CFG
    ldr     r1, [r0]
    orr     r1, r1, #0x600
    str     r1, [r0]

    pop     {r4, pc}

/*
 * pinmux_config(r0 = pin, r1 = function)
 *
 * Reimplementation of SEPROM 0x10005C64.  Each function updates
 * (PINMUX_BASE + pin*4) as:
 *      new = (old & mask) | set
 * with the mask/set pairs from the ROM's TBB table.
 */
pinmux_config:
    cmp     r1, #6
    bhi     9f
    adr     r2, pinmux_table
    add     r2, r2, r1, lsl #3
    ldr     r3, [r2, #0]
    ldr     r12, [r2, #4]
    MMIO_REG r1, PINMUX_BASE
    add     r1, r1, r0, lsl #2
    ldr     r0, [r1]
    and     r0, r0, r3
    orr     r0, r0, r12
    str     r0, [r1]
9:  bx      lr

.balign 4
pinmux_table:
    .word 0xF8F8FF91, 0x07000200   @ func 0
    .word 0xF8F8FF90, 0x07000202   @ func 1
    .word 0xF8F8FF90, 0x07000203   @ func 2
    .word 0xF8F8FF91, 0x07000202   @ func 3
    .word 0xF8F8FF91, 0x07000220   @ func 4
    .word 0xF8F8FF91, 0x07000240   @ func 5
    .word 0xF8F8FF91, 0x07000260   @ func 6

/*
 * delay_ticks(r0 = ticks)
 *
 * SEPROM waits on the CP15 virtual counter.  The exact rate is not
 * relevant for pinmux settling, so use a bounded software delay instead
 * of depending on the timer being enabled in this context.
 */
delay_ticks:
    lsl     r0, r0, #6
1:  subs    r0, r0, #1
    bne     1b
    bx      lr

/*
 * eeprom_read_header16(r0 = 16-byte buffer)
 *
 * Reimplementation of SEPROM's sep_eeprom_read(offset=0, len=16) /
 * sep_i2c_read_transaction for slave 0xA0, one address byte.
 * Returns 0 on success, 1 on controller error/timeout.
 */
eeprom_read_header16:
    push    {r4-r7, lr}
    mov     r4, r0

    MMIO_REG r5, I2C_CMD
    MMIO_REG r6, I2C_CTRL

    /* 0x3D700014 = 0x08200000 (ROM transaction setup). */
    movw    r0, #0x0000
    movt    r0, #0x0820
    str     r0, [r6]

    /* Slave 0xA0, then one address byte (0x00), then read command. */
    movw    r0, #0x01A0             @ (slave & 0xFE) | 0x100
    str     r0, [r5]
    mov     r0, #0
    str     r0, [r5]                @ EEPROM address byte 0
    movw    r0, #0x01A1             @ 0x101 | slave
    str     r0, [r5]
    movw    r0, #0x0610             @ byte count 16 | command 6 << 8
    str     r0, [r5]
    dsb     sy

    MMIO_REG r7, I2C_STS
    mov     r2, #16

.Lbyte_loop:
    movw    r3, #0xFFFF
    movt    r3, #0x000F             @ bounded poll (~1M)

.Lpoll_status:
    ldr     r0, [r7]
    tst     r0, #0x100              @ bit8 set while byte not ready
    beq     .Lbyte_ready
    ldr     r1, [r6]
    tst     r1, #0x200000           @ controller error
    bne     .Li2c_error
    subs    r3, r3, #1
    bne     .Lpoll_status
    b       .Li2c_error

.Lbyte_ready:
    strb    r0, [r4], #1
    subs    r2, r2, #1
    bne     .Lbyte_loop

    mov     r0, #0
    pop     {r4-r7, pc}

.Li2c_error:
    /* Same clear sequence as SEPROM's error path. */
    MMIO_REG r0, I2C_CFG
    ldr     r1, [r0]
    orr     r1, r1, #0x200
    str     r1, [r0]
    MMIO_REG r0, I2C_CTRL
    movw    r1, #0x0000
    movt    r1, #0x0020
    str     r1, [r0]
    mov     r0, #1
    pop     {r4-r7, pc}

/* ------------------------------------------------------------------ */
/* Status report                                                       */
/* ------------------------------------------------------------------ */

/*
 * report_status(r0 = 0 ok / 1 I2C / 2 KDF)
 *
 * Send one SEP->AP mailbox message so the loader can distinguish
 * "key written" from "payload ran but failed".  Opcodes 0xAD/0xAE/0xAF
 * are not handled specially by PongoOS, so they do not disturb the
 * peek event used to read 0x10140000 afterwards.
 */
report_status:
    MMIO_REG r2, MAILBOX_BASE
1:  ldr     r3, [r2]
    tst     r3, #0x10000            @ FULL
    bne     1b
    movw    r1, #0x00FF
    movt    r1, #0x00AD
    add     r1, r1, r0, lsl #16     @ 0xAD / 0xAE / 0xAF
    str     r1, [r2, #0x10]
    mov     r1, #0
    str     r1, [r2, #0x14]
    dsb     sy
    bx      lr

.section .note.GNU-stack,"",%progbits
