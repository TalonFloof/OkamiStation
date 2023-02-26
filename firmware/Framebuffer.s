.text
.global FramebufferClear: /* (color: a0) */
    la t0, 0x90001000
    la t1, 0x900c1000
.loop:
    sb a0, 0(t0)
    addi t0, t0, 1
    bltu t0, t1, .loop
    blr zero, ra

/*.global FramebufferRenderMonochromeBitmap:*/ /* (x y w h scale color invert ptr) */
    /* thx ry, ur the best :3 */
/*    addi sp, sp, -24
    sw ra, 4(sp)
    sw a0, 8(sp)
    sw a1, 12(sp)
    sw a2, 16(sp)
    sw a3, 20(sp)
    sw a4, 24(sp)

    add t0, zero, zero
    add t1, zero, zero
    addi t2, zero, 8
.loop:
    addi t3, zero, 1
    sll t3, t3, t2
    lb t4, 0(a7)
    and t3, t4, t3
    beq t3, zero, .after
    lw a0, 8(sp)
    lw a1, 12(sp)
    add a0, a0, t0
    mulu a0, zero, a0, a4
    add t7, a1, zero
    add a1, a1, zero
    beq a6, zero, .no_invert
    lw a3, 20(sp)
    sub a1, a3, a1
    mulu a1, zero, a1, a4
    beq zero, zero, .after_no_invert
.no_invert:
    mulu a1, zero, a1, a4
.after_no_invert:
    add a1, a1, t7
    add a2, a6, a6
    add a3, a6, a6
    blr ra, FramebufferDrawRect
.after:
    addi t2, t2, -1
    bltu zero, t2, .loop

    addi t2, zero, 8
    addi a7, a7, 1
    addi t1, t1, 1
    bltu t1, a2, .loop

    add t1, zero, zero
    addi t0, t0, 1
    bltu t0, a3, .loop

    lw a4, 24(sp)
    lw a3, 20(sp)
    lw a2, 16(sp)
    lw a1, 12(sp)
    lw a0, 8(sp)
    lw ra, 4(sp)
    addi sp, sp, 24
    blr zero, ra*/

.rodata
OkamiLogo: .include_bin "Images/OkamiLogo.dat"

RAMIcon: .include_bin "Images/RAMIcon.dat"

Unifont: .include_bin "Images/Unifont.dat"

Dither: .include_bin "Images/Dither.dat"
.text
