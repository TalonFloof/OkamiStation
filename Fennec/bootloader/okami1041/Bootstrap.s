/*
 *  This program assumes that the Fennec Kernel is loaded on inode 2, DO NOT MAKE THE KERNEL FILE ANYTHING OTHER THAN THAT!
 */

.text
.global _start:
    la a0, banner
    mcall 0x101
    la t7, blockbuf
    mv t7, t6
    addi t6, t6, 2048
    li a0, 64 /* Read the Superblock */
    li a1, 1
    mv t7, a2
    mcall 0x201
    lw a0, 4(t7) /* Read the first Inode Block */
    addi a0, a0, 64
    li a1, 1
    mv t6, a2
    mcall 0x201
    mv t6, t5
    addi t5, t5, 256
    lw t0, 48(t5) /* Get the first zone */
.halt:
    beq zero, zero, .halt

.global ReadZone: /* (a0: Zone, a2: Address) | (a0: Next Zone) */
    addi sp, sp, -4
    sw a0, 4(sp)
    lw t0, 24(t7)
    srli t0, t0, 9

    lw t0, 16(t7)
    addi t0, t0, 64

    addi sp, sp, 4
    lw a0, 0(sp)
    br ra

.rodata
banner: .ascii "Fennec Bootloader (for OkamiStation 1000)" .byte 0xa .ascii "(C) 2023 TalonFox, Licensed under the MIT License" .byte 0xa .byte 0xa .byte 0
.bss
blockbuf: .resb 2048