okbootClear:
    li a0, 2
    mcall 0x104
    br ra

okbootPrint:
    mcall 0x101
    br ra
