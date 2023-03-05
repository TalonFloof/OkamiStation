.text
.global OkamiStationFirmwareStartup:
    la a0, EarlyHandler
    lui t1, 0x2000
    add a0, a0, t1
    mtex a0, 5
    mtex a0, 6
    lw zero, 1(zero)
    /* We must first clear the caches */
    /* Assembler will relocate to 0x9ff0_0000 as base, we need to fix that. */
    la t0, ClearCaches
    lui t1, 0x2000
    add t0, t0, t1
    blr ra, t0
    /* Caches are now cleared, clear the TLB next */
    la t0, ClearTLB
    lui t1, 0x2000
    add t0, t0, t1
    blr ra, t0
    /* Setup Stack */
    la sp, FWbss_end
    la t0, main
    br t0
halt:
    beq zero, zero, halt

.global ClearCaches:
    /* Clear Instruction Cache */
    lui t1, 0x8000
    la t2, 0x80004000
    mfex t0, 0x00 /* OKAMI_STATUS */
    ori t0, t0, 0x18
    mtex t0, 0x00 /* OKAMI_STATUS */
.icache_loop:
    sw zero, 4(t1)
    addi t1, t1, 8
    bltu t1, t2, .icache_loop
    /* Clear Data Cache */
    lui t1, 0x8000
    andi t0, t0, 0xffef
    mtex t0, 0x00 /* OKAMI_STATUS */
.dcache_loop:
    sw zero, 4(t1)
    addi t1, t1, 8
    bltu t1, t2, .dcache_loop
    andi t0, t0, 0xfff7
    mtex t0, 0x00 /* OKAMI_STATUS */
    blr zero, ra

.global ClearTLB:
    li t0, 0
    li t1, 64
.loop:
    mtex t0, 0x10 /* OKAMI_TLB_INDEX */
    mtex zero, 0x11 /* OKAMI_TLB_VALUE_LOW */
    mtex zero, 0x12 /* OKAMI_TLB_VALUE_HIGH */
    addi t0, t0, 1
    bltu t0, t1, .loop
    blr zero, ra