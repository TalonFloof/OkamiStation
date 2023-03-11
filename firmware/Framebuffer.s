.text
FramebufferDither: /* (a0: Color) */
    la t0, 0x90001000
    lui t1, 0xc /*786432*/
    add t1, t0, t1
.loop:
    sb a0, 0(t0)
    addi t0, t0, 2
    bltu t0, t1, .loop
    br ra

.rodata
OkamiLogo: .include_bin "Images/OkamiLogo.dat"

Unifont: .include_bin "Images/Unifont.dat"
.align 4
Dither: .include_bin "Images/Dither.dat"
.text