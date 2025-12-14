.syntax unified
.cpu cortex-a8
.arm
.global get_threshold
.type get_threshold, %function

get_threshold:
        ldr     ip, .L4
        umull   r0, r3, ip, r0
        push    {r4, lr}
        ldr     r4, .L4+4
        ldr     r2, .L4+8
        adds    r0, r0, r4
        mla     r1, ip, r1, r3
        umull   r0, r3, r2, r0
        ldr     lr, .L4+12
        adc     r1, r1, #0
        umlal   r0, r3, r1, lr
        mov     r0, #0
        umlal   r3, r0, r1, r2
        lsr     r3, r3, #26
        orr     r3, r3, r0, lsl #6
        subs    r2, r3, #52
        sbc     r0, r0, r0
        bic     r3, r3, r0
        and     r0, r0, #52
        orr     r0, r0, r3
        and     r0, r0, #255
        pop     {r4, lr}
        bx      lr

.L4:
        .word   526500
        .word   1525880000
        .word   -1412663535
        .word   -2073964803
