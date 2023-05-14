.text
.global OkamiTLBMiss: /* Fast TLB Refill Routine */
    mfex k1, 0x15 /* OKAMI_TLB_CONTEXT -> k1 */
    lw k1, 0(k1) /* This will double fault if there is no Page Table
                    TLB Entry within the Wired entries (Entries 0-7) */
    mtex k1, 0x11 /* k1 -> OKAMI_TLB_LOW */
    tlbwr /* Write OKAMI_TLB_LOW to a random TLB entry */
    rft /* Return to userspace */

.global OkamiNestedTLBMiss: /* Slow TLB Refill Routine */
    /* Note: k1 is saved in OKAMI_TRAP_KERNEL_SCRATCH by the general trap vector */
    mfex k1, 0x15 /* OKAMI_TLB_CONTEXT -> k1 */
    la k0, 0x1FFFFC
    and k1, k1, k0
    mfex k0, 0x16 /* OKAMI_TLB_PAGE_DIRECTORY -> k0 */
    add k0, k0, k1
    lw k0, 0(k0)
    mtex k0, 0x11 /* k0 -> OKAMI_TLB_LOW */
    mfex k1, 0x13 /* OKAMI_TLB_RANDOM_INDEX -> k1 */
    andi k1, k1, 7
    mtex k1, 0x10 /* k1 -> OKAMI_TLB_INDEX */
    tlbwi /* Write OKAMI_TLB_LOW to the indexed TLB entry */
    mfex k1, 0x4 /* OKAMI_TRAP_KERNEL_SCRATCH -> k1 */
    rft /* Return to userspace */
