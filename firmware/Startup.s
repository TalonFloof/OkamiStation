.text
.global OkamiStationFirmwareStartup:
    /* We must first clear the caches */
    la t0, ClearCaches /* Assembler will relocate to 0x8000_0000 as base, we need to fix that. */
    lui t1, 0x3ff0
    add t0, t0, t1
    blr ra, t0
    /* Caches are now cleared, reset the KoriBus Controller */
    /* Copy to RAM */
    lui t0, 0xa000
    lui t1, 0xbff0
    lui t2, 0xbff2
.memcpy:
    lw t3, 0(t1)
    addi t1, t1, 4
    sw t3, 0(t0)
    addi t0, t0, 4
    bltu t1, t2, .memcpy
    /* Setup Stack */
    la sp, 0x803ffffc
    la t0, main
    blr zero, t0
.halt:
    beq zero, zero, .halt

.global ClearCaches:
    /* Clear Instruction Cache */
    lui t1, 0x8000
    la t2, 0x80008000
    mfex t0, 0x00
    ori t0, t0, 0x18
    mtex t0, 0x00
.icache_loop:
    sw zero, 4(t1)
    addi t1, t1, 8
    bltu t1, t2, .icache_loop
    /* Clear Data Cache */
    lui t1, 0x8000
    andi t0, t0, 0xffef
    mtex t0, 0x00
.dcache_loop:
    sw zero, 4(t1)
    addi t1, t1, 8
    bltu t1, t2, .dcache_loop
    andi t0, t0, 0xfff7
    mtex t0, 0x00
    blr zero, ra