.text
.global _start:
    la a0, banner
    mcall 0x101
    bl getBootOptions
.halt:
    beq zero, zero, .halt
.global getBootOptions:
    li a0, 153
    mcall 0x102
    la a0, bootPrompt
    mcall 0x101
    li a0, 253
    mcall 0x102
    li t0, 0
    la t2, bootPromptData
.loop:
    mcall 0x103
    li t1, 8
    beq a1, t1, .backspace
    li t1, 0xa
    beq a1, t1, .ret
    mv a1, a0
    mcall 0x100
    addi t0, t0, 1
    sb a1, 0(t2)
    addi t2, t2, 1
    beq zero, zero, .loop
.backspace:
    beq t0, zero, .loop
    addi t0, t0, -1
    addi t2, t2, -1
    sb zero, 0(t2)
    mv a1, a0
    mcall 0x100
    beq zero, zero, .loop
.ret:
    mv a1, a0
    mcall 0x100
    sb zero, 0(t2)
    br ra
.rodata
banner: .ascii "Fennec Bootloader (for OkamiStation 1000)" .byte 0xa .ascii "(C) 2023 TalonFox, Licensed under the MIT License" .byte 0xa .byte 0xa .byte 0
bootPrompt: .string "boot: "
.data
bootPromptData: .fill 128,0