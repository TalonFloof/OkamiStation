.text
.global _start:
    la a1, hello
    mcall 0x101
    beq zero, zero, -1
.rodata
hello: .string "Hello world!"