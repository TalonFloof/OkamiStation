/* Reg Base: 0xbe000080 */
SCSISelect: /* (a0: ID) */
    la t0, 0xbe000080 /* Write SCSI ID to Data Out*/
    sw a0, 0(t0)
    lw t1, 8(t0) /* Enable the Arbitration Bit */
    ori t1, t1, 1
    sw t1, 8(t0)
    li t2, 0
    la t3, 16777216
.arbinloop:
    lw t1, 4(t0)
    andi t1, t1, 0x40
    addi t2, t2, 1
    bgeu t2, t3, .err1
    beq t1, zero, .arbinloop
    beq zero, zero, .lostarbloop
.err1:
    li a0, 1
    br ra
.lostarbloop:
    lw t1, 4(t0)
    andi t1, t1, 0x20
    beq t1, zero, .arbsuccess
    /* Reset the Arbitration Bit and try again */
.arbsuccess:
    li a0, 0
    br ra