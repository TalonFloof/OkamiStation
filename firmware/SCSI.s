/* Reg Base: 0xbe000080 */
SCSISelect: /* (a0: Initiator ID a1: Target ID, a2: Reselect) */
    la t0, 0xbe000080 /* Write Initiator ID to Data Out*/
    sw a0, 0(t0)
    lw t1, 8(t0) /* Enable the Arbitration Bit */
    ori t1, t1, 1
    sw t1, 8(t0)
.arbinloop:
    lw t1, 4(t0)
    andi t1, t1, 0x40
    beq t1, zero, .arbinloop
.lostarbloop:
    lw t1, 4(t0)
    andi t1, t1, 0x20
    beq t1, zero, .select
    /* Reset the Arbitration Bit and try again */
    lw t1, 8(t0) /* Enable the Arbitration Bit */
    andi t1, t1, 0xfe
    sw t1, 8(t0)
    ori t1, t1, 1
    sw t1, 8(t0)
    beq zero, zero, .arbinloop
.select:
    lw t1, 4(t0) /* Set the SEL pin to high */
    ori t1, t1, 4
    sw t1, 4(t0)
    bne a1, zero, .after_reselect1
    /* Do reselect specific stuff */
    lw t1, 8(t0)
    ori t1, t1, 0x40
    sw t1, 8(t0)
    lw t1, 12(t0)
    ori t1, t1, 1
    sw t1, 12(t0)
.after_reselect1:
    or 
    li a0, 0
    br ra