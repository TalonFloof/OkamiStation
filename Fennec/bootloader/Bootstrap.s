.text
.global _start:
    la a0, hello
    mcall 0x101
.halt:
    beq zero, zero, .halt
.rodata
hello: .ascii "Hello world!" .byte 0xa .byte 0