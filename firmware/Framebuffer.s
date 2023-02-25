.text
.global FramebufferClear: /* (color: a0) */
    la t0, 0x90001000
    la t1, 0x900c1000
.loop:
    sb a0, 0(t0)
    addi t0, t0, 1
    bltu t0, t1, .loop
    blr zero, ra

.rodata
OkamiLogo: .include_bin "Images/OkamiLogo.dat"

RAMIcon: .include_bin "Images/RAMIcon.dat"

Unifont: .include_bin "Images/Unifont.dat"

Dither: .include_bin "Images/Dither.dat"
.text
