.text
__start:
    la k0, __exceptionHandler
    mtex k0, 5 /* OKAMI_TRAP_VECTOR_OFFSET */
    la k0, OkamiTLBMiss
    mtex k0, 6 /* OKAMI_TLB_MISS_VECTOR_OFFSET */
    lui k0, 0xc000
    mtex k0, 0x15 /* OKAMI_TLB_CONTEXT */
    b HALMain
