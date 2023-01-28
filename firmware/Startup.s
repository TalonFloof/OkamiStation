.text
_OkamiStationFirmwareStartup:
    /* We must first clear the caches */

    /* Clear Instruction Cache */
    la t1, 0x800000006
    la t2, 0x80004000
    mfex t0, 0x00
    ori t0, t0, 0x18
    mtex t0, 0x00
.icache_loop:
    sw zero, 0(t1)
    sw zero, 4(t1)
    addi t1, t1, 8
    blt t1, t2, .icache_loop
    