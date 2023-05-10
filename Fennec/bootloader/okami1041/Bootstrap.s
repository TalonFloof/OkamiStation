/*
 *  This program assumes that the Fennec Kernel is loaded on inode 2, DO NOT MAKE THE KERNEL FILE ANYTHING OTHER THAN THAT!
 */

.text
.global _start:
    la a0, banner
    mcall 0x101
    la s9, blockbuf
    mv s9, s8
    addi s8, s8, 512
    li a0, 4 /* Read the Superblock */
    li a1, 1
    mv s9, a2
    mcall 0x201
    lw a0, 4(s9) /* Read the first Inode Block */
    addi a0, a0, 4
    li a1, 1
    mv s8, a2
    mcall 0x201
    lw a0, 304(s8) /* Get the first zone */
    addi t1, zero, -2 /* 0xfffffffe */
    la a2, 0x80005000
.loop:
    bl ReadZone
    bltu a0, t1, .loop
    la t0, 0x80005000
    br t0

.global ReadZone: /* (a0: Zone, a2: Address) | (a0: Next Zone, a2: Next Address) Clobers: t0 */
    addi sp, sp, -12
    sw a0, 4(sp)
    sw a1, 8(sp)
    sw a2, 12(sp)
    
    lw a1, 0x18(s9) /* Calculate the sector start and number of sectors */
    srli a1, a1, 9
    lw t0, 0x10(s9)
    addi t0, t0, 4
    mulu a0, zero, a0, a1
    add a0, a0, t0
    mcall 0x201 /* Read the sectors */
    
    lw a0, 4(sp) /* Read the ZZT sector with the entry */
    srli a0, a0, 7
    lw t0, 0x20(s9)
    addi t0, t0, 4
    add a0, a0, t0
    li a1, 1
    mv s8, a2
    mcall 0x201
    lw a0, 4(sp)
    andi a0, a0, 0x7f
    slli a0, a0, 2
    add a0, a0, s8
    lw a0, 0(a0)
    addi a0, a0, -1
    
    lw a1, 8(sp)
    lw a2, 12(sp)
    lw t0, 0x18(s9)
    add a2, a2, t0
    addi sp, sp, 12
    br ra

.rodata
banner: .ascii "Fennec Bootloader (for OkamiStation 1000)" .byte 0xa .ascii "(C) 2023 TalonFox, Licensed under the MIT License" .byte 0xa .byte 0xa .byte 0
.bss
blockbuf: .resb 1024