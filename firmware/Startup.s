.text
_OkamiStationFirmwareStartup:
    ; We must first clear the caches

    ; Clear Instruction Cache
    li t1, 0
    mfex t0, 0x00
    ori t0, t0, 0x18
    mtex t0, 0x00
.icache_loop:
    
    addi t1, t1, 16