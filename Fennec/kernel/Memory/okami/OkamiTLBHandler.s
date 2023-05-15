.text
.global OkamiTLBMiss: /* Fast TLB Refill Routine */
    mfex k0, 0x2 /* OKAMI_TRAP_PC -> k0 */
    mfex k1, 0x15 /* OKAMI_TLB_CONTEXT -> k1 */
    lw k1, 0(k1) /* This will double fault if there is no Page Table
                    TLB Entry within the Wired entries (Entries 0-7) */
    mtex k1, 0x11 /* k1 -> OKAMI_TLB_LOW */
    tlbwr /* Write OKAMI_TLB_LOW to a random TLB entry */
    rft /* Return to userspace */

.global OkamiNestedTLBMiss: /* Slow TLB Refill Routine */
    mfex k0, 0x15 /* OKAMI_TLB_CONTEXT -> k1 */
    la k1, 0xffefffff
    and k0, k0, k1
    lw k0, 0(k0)
    mtex k0, 0x11 /* k0 -> OKAMI_TLB_LOW */
    mfex k1, 0x13 /* OKAMI_TLB_RANDOM_INDEX -> k1 */
    li k0, 7
    divu zero, k1, k1, k0
    addi k1, k1, 1
    mtex k1, 0x10 /* k1 -> OKAMI_TLB_INDEX */
    tlbwi /* Write OKAMI_TLB_LOW to the indexed TLB entry */
    mfex k0, 0x4 /* OKAMI_TRAP_KERNEL_SCRATCH -> k0 */
    mtex k0, 0x2 /* k0 -> OKAMI_TRAP_PC */
    rft /* Return to the Fast TLB Refill Routine */
