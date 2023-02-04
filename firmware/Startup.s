.text
.global OkamiStationFirmwareStartup:
    /* We must first clear the caches */
    la t0, ClearCaches /* Assembler will relocate
                        * to 0x8000_0000 as base, we need to fix that. 
                        */
    la t1, 0x7c000000
    add t0, t0, t1
    blr ra, t0
    /* Caches are now cleared, reset the KoriBus Controller */
    /* Copy to RAM */
    

.global ClearCaches:
    /* Clear Instruction Cache */
    la t1, 0x80000000
    la t2, 0x80008000
    mfex t0, 0x00
    ori t0, t0, 0x18
    mtex t0, 0x00
.icache_loop:
    sw zero, 4(t1)
    addi t1, t1, 8
    blt t1, t2, .icache_loop
    /* Clear Data Cache */
    la t1, 0x80000000
    andi t0, t0, 0xffef
    mtex t0, 0x00
.dcache_loop:
    sw zero, 4(t1)
    addi t1, t1, 8
    blt t1, t2, .dcache_loop
    andi t0, t0, 0xfff7
    mtex t0, 0x00
    blr zero, ra