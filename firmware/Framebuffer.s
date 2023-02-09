.text
.global FramebufferClear: /* (color: a0) */
    la t0, 0xb0001000
    la t1, 0xb00c1000
.loop:
    sb a0, 0(t0)
    addi t0, t0, 1
    bltu t0, t1, .loop
    blr zero, ra

/* (x: a0, y: a1, w: a2, h: a3, color: a4) */
/*FramebufferDrawRect:
    li t0, 1024
    mulu t0, zero, a1, t0
    add t0, t0, a0
    la t1, 0xb0001000
    add t0, t0, t1
    li t1, 0
    li t2, 0
.loop:
    sb a4, 0(t0)
    addi t0, t0, 1
    addi t1, t1, 1
    bltu t1, a2, .loop
    li t1, 0
    addi t0, t0, 1024
    sub t0, t0, a2
    addi t2, t2, 1
    bltu t2, a3, .loop
    blr zero, ra*/

.rodata
OkamiLogo: .include_bin "Images/OkamiLogo.dat"

RAMIcon: .include_bin "Images/RAMIcon.dat"

Unifont: .include_bin "Images/Unifont.dat"
.text
