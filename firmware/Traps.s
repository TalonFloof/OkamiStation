.global SetupTrap: /* (addr: a0, type: a1) */
    li t0, 1
    beq a1, t0, .tlbSetup
    li t0, 2
    beq a1, t0, .mcallSetup
    bltu t0, a1, .ret /* bgtu a1, t0, .ret */
    mtex a0, 5 /*  OKAMI_TRAP_VECTOR_OFFSET */
    beq zero, zero, .ret
.tlbSetup:
    mtex a0, 6 /* OKAMI_TLB_MISS_VECTOR_OFFSET */
    beq zero, zero, .ret
.mcallSetup:
    mtex a0, 7 /* OKAMI_MCALL_VECTOR_OFFSET */
    beq zero, zero, .ret
.ret:
    br ra

.global TrapHandler:
    addi sp, sp, -76
    sw a0, 4(sp)
    sw a1, 8(sp)
    sw a2, 12(sp)
    sw a3, 16(sp)
    sw a4, 20(sp)
    sw a5, 24(sp)
    sw a6, 28(sp)
    sw a7, 32(sp)
    sw s0, 36(sp)
    sw s1, 40(sp)
    sw s2, 44(sp)
    sw s3, 48(sp)
    sw s4, 52(sp)
    sw s5, 56(sp)
    sw s6, 60(sp)
    sw s7, 64(sp)
    sw s8, 68(sp)
    sw s9, 72(sp)
    sw ra, 76(sp)
    addi a0, sp, 4
    mfex a1, 1
    mfex a2, 2
    mfex a3, 3
    bl Exception
    b halt