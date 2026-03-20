.syntax unified
.cpu cortex-a8
.arm
.global get_threshold3
.type get_threshold3, %function

get_threshold3:
        ldr     r2, .L3
        ldr     ip, .L3+4
        umull   r0, r3, r0, r2
        mla     r1, r2, r1, r3
        ldr     r3, .L3+8
        ldr     r2, .L3+12
        adds    r0, r0, r3
        adc     r1, r1, #0
        umull   r0, r3, r0, r2
        umlal   r0, r3, r1, ip
        movs    r0, #0
        umlal   r3, r0, r1, r2
        lsrs    r3, r3, #26
        orr     r3, r3, r0, lsl #6
#ifndef FAULT_A
        subs    r2, r3, #52
#endif
        sbc     r0, r0, r0
        mvns    r2, r0
        and     r0, r0, #52
#ifndef FAULT_B
        ands    r2, r2, r3
#endif
        orrs    r0, r0, r2
        uxtb    r0, r0
        bx      lr
.L3:
        .word   526500
        .word   -2073964803
        .word   1525880000
        .word   -1412663535

.section .note.GNU-stack,"",%progbits
