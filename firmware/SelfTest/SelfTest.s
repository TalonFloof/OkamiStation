TestRAM_:
    la a0, __BSS_END__
    la t2, RAMTestFaulted
    li t3, 2
    lui t0, 0x2000
    add a0, a0, t0
.loop:
    la t0, 0x55555555
    sw t0, 0(a0)
    lbu t1, 0(t2)
    beq t1, t3, .end
    lw t1, 0(a0)
    bne t1, t0, .badram
    la t0, 0xaaaaaaaa
    sw t0, 0(a0)
    lw t1, 0(a0)
    bne t1, t0, .badram
    addi a0, a0, 4
    beq zero, zero, .loop
.badram:
    bl RAMErr    
.end:
    br ra

.rodata .byte 0x58 .byte 0x59 .byte 0x5A .byte 0x5A .byte 0x59 .byte 0 .text