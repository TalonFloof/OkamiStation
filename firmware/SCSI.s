/* Reg Base: 0xbe000080 */
SCSISelect: /* (a0: ID) */
    la t0, 0xbe000080 /* Write SCSI ID to Data Out*/
    sw a0, 0(t0)
    lw t1, 8(t0) /* Enable the Arbitration Bit */
    ori t1, t1, 1
    sw t1, 8(t0)
.arbinloop:
    lw t1, 4(t0)
    andi t1, t1, 0x40
    beq t1, zero, .arbinloop
    beq zero, zero, .lostarbloop
.lostarbloop:
    lw t1, 4(t0)
    andi t1, t1, 0x20
    beq t1, zero, .arbsuccess
    /* Reset the Arbitration Bit and try again */
    lw t1, 8(t0) /* Enable the Arbitration Bit */
    andi t1, t1, 0xfe
    sw t1, 8(t0)
    ori t1, t1, 1
    sw t1, 8(t0)
    beq zero, zero, .arbinloop
.arbsuccess:
    li a0, 0
    br ra