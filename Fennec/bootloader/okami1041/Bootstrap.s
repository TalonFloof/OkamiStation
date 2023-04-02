.text
.global _start:
    la a0, banner
    mcall 0x101
    la a0, 
.halt:
    beq zero, zero, .halt
.rodata
banner: .ascii "Fennec Bootloader (for OkamiStation 1000)" .byte 0xa .ascii "(C) 2023 TalonFox, Licensed under the MIT License" .byte 0xa .byte 0xa .byte 0
.bss
blockbuf: .resb 512