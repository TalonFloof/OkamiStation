.text
.global memset: /* (base: a0, size: a1, value: a2) */
    beq a1, zero, .ret
    li t0, 8
    bltu a1, t0, .loop8
    andi t0, a0, 3
    beq t0, zero, .begin32
    li t1, 4
    sub t0, t1, t0
.loop8:
    sb a2, 0(a0)
    addi a0, a0, 1
    addi a1, a1, -1
    addi t0, t0, -1
    beq a1, zero, .ret
    bltu zero, t0, .loop8 /* t0 > zero */
.begin32:
    add t0, a2, zero
    slli a2, a2, 8
    or a2, a2, t0
    slli a2, a2, 8
    or a2, a2, t0
    slli a2, a2, 8
    or a2, a2, t0
    li t0, 3
.loop32:
    sw a2, 0(a0)
    addi a0, a0, 4
    addi a1, a1, -4
    bltu t0, a1, .loop32 /* a1 > 3 */
    bne a1, zero, .loop8
.ret:
    br ra

.global memcpy: /* (dest: a0, src: a1, len: a2) */
    lb t0, 0(a1)
    sb t0, 0(a0)
    addi a0, a0, 1
    addi a1, a1, 1
    addi a2, a2, -1
    bne a2, zero, memcpy
.ret:
    br ra