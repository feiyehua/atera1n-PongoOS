// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 feiyehua

.syntax unified
.arch armv7-a
.arm
.text
.balign 4
.global _start

.type _start, %function
_start:
    andsmi r8, r0, r0, lsr r3
    dsb     sy
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0   @ TLBIALL
    dsb     sy
    isb     sy

    mrc     p15, 0, r0, c1, c0, 0   @ Read SCTLR
    orr     r0, r0, #1               @ Set SCTLR.M
    mcr     p15, 0, r0, c1, c0, 0
    isb     sy

    mov     r0, #0
    movw    r1, #0x00ff
    movt    r1, #0x00ab              @ r1 = 0x00AB00FF
    bl      sep_send
.Ldone:
    bl      sep_recv
    ubfx    r2, r0, #16, #8    @ get opcode
    cmp     r2, #0xC0
    beq     .Lpeek
    cmp     r2, #0xC1          @ opcode 0xC1 recieve poke location
    beq     .Lpoke
    cmp     r2, #0xC3          @ opcode 0xC3 recieve branch location to r1
    beq     .Lbranch
    b       .Ldone
.Lpeek:
    ldr     r0, [r1]
    movw    r1, #0x00ff
    movt    r1, #0x00ac              @ r1 = 0x00AC00FF
    bl      sep_send
    b       .Ldone
.Lpoke:
    mov     r3, r1             @ poke location 
.Lrecv_next:
    bl      sep_recv
    ubfx    r2, r0, #16, #8    @ get opcode
    cmp     r2, #0xC2          @ opcode 0xC2 recieve poke data to r1
    bne     .Lrecv_next
    str     r1, [r3]
    b       .Ldone
.Lbranch:
    blx      r1

    b       .Ldone
.size _start, . - _start

@ r0 32 bit data
@ r1 32 bit endpoint and opcode
.type sep_send, %function
sep_send:
    movw    r2, #0x0ba0
    movt    r2, #0x3da0              @ Mailbox VA 0x3DA00BA0

.Lcheck_full:
    ldr     r3, [r2]
    tst     r3, #0x10000             @ FULL
    bne     .Lcheck_full

.Lsend:
    str     r1, [r2, #0x10]          @ Low 32 bits
    str     r0, [r2, #0x14]          @ High 32 bits
    bx      lr
.size sep_send, . - sep_send


.equ SEP_MAILBOX_BASE,    0x3DA00B80
.equ SEP_RX_EMPTY,        0x20000

.type sep_recv, %function

@ Receive one AP-to-SEP message. Returns low word in r0, high word in r1.
@ Requires the T7000 SEP MMIO mapping. Polls until a message arrives.
sep_recv:
    movw    r2, #:lower16:SEP_MAILBOX_BASE
    movt    r2, #:upper16:SEP_MAILBOX_BASE

.Lcheck_rx:
    ldr     r0, [r2, #0x08]          @ RX status
    tst     r0, #SEP_RX_EMPTY
    bne     .Lcheck_rx

    ldr     r0, [r2, #0x18]          @ Low 32 bits
    ldr     r1, [r2, #0x1c]          @ High 32 bits
    bx      lr
.size sep_recv, . - sep_recv