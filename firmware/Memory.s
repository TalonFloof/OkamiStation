.global memset: /* (base: a0, size: a1, value: a2) */
    sb a2, 0(a0)
    addi a0, a0, 1
    addi a1, a1, -1
    bltu zero, a1, memset /* a1 > zero */
    blr zero, ra