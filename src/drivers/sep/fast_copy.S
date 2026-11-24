.text
.align 4
.globl _fast_copy
_fast_copy:
Lcopy64_loop:
    cmp     x2, #0x40
    b.lo    Lcopy64_done

    ldp     q0, q1, [x1]
    ldp     q2, q3, [x1, #0x20]

    stp     q0, q1, [x0]
    stp     q2, q3, [x0, #0x20]

    add     x0, x0, #0x40
    add     x1, x1, #0x40
    sub     x2, x2, #0x40

    b       Lcopy64_loop

Lcopy64_done:
    dmb     sy
    ret