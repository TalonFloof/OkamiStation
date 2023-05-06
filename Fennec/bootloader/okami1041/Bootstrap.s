/*
 *  This program assumes that the Fennec Kernel is loaded on inode 2, DO NOT MAKE THE KERNEL FILE ANYTHING OTHER THAN THAT!
 */

.text
.global _start:
    la a0, banner
    mcall 0x101
    li a0, 64 /* Read the Superblock */
    li a1, 1
    la a2, blockbuf
    mcall 0x200
.halt:
    beq zero, zero, .halt
.rodata
banner: .ascii "Fennec Bootloader (for OkamiStation 1000)" .byte 0xa .ascii "(C) 2023 TalonFox, Licensed under the MIT License" .byte 0xa .byte 0xa .byte 0
.bss
blockbuf: .resb 512