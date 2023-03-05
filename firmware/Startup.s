.text
.global OkamiStationFirmwareStartup:
    /* We must first clear the caches */
    /* Assembler will relocate to 0x9ff0_0000 as base, we need to fix that. */
    la a0, EarlyHandler
    lui t1, 0x2000
    add a0, a0, t1
    mtex a0, 5
    bl ClearCaches
    la a0, 0xb0001000
    li a1, 786432
    li a2, 0
    bl memset
    /* Caches are now cleared, clear the TLB next */
    bl ClearTLB
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
    sw zero, 0(t1)
    sw zero, 4(t1)
    addi t1, t1, 8
    bltu t1, t2, .icache_loop
    /* Clear Data Cache */
    lui t1, 0x8000
    andi t0, t0, 0xffef
    mtex t0, 0x00 /* OKAMI_STATUS */
.dcache_loop:
    sw zero, 0(t1)
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