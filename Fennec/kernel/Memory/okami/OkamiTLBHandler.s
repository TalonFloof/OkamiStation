.text
.global OkamiTLBMiss:
    mfex k1, 4 /* OKAMI_TRAP_KERNEL_SCRATCH */
    mfex k0, 5 /* OKAMI_TRAP_BAD_VADDR */
    lw k0, 0(k0)
    srli k0, k0, 12
    slli k0, k0, 2
    add k0, k0, k1
    tlbwr
    rft