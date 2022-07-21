; uint64_t bank_test(bank_base -> a0);
bank_test:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push t0, t1, t2
    sub 8
    sd %t0
    sub 8
    sd %t1
    sub 8
    sd %t1
    ;;;;;;;;;;;;;;;;;;;
    mov $bank_test_trap, %t2
    as %t2
    or 1
    mov 0x20, %xreg.ind
    mov %t2, %xreg.val
    mov 0, %t0
    mov 0, %t2
bank_test.loop:
    as %a0
    sd %zero
    fence 0
    bne %t2, %zero, @bank_test.end
    add 8
    as %t0
    add 8
    b @bank_test.loop
bank_test.end:
    mov %t0, %a0
    mov 0x20, %xreg.ind
    mov %zero, %xreg.val
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; pop t2, t1, t0
    ld %t2
    add 8
    ld %t1
    add 8
    ld %t0
    add 8
    ;;;;;;;;;;;;;;;;;;;
    ba %ra

bank_test_trap:
    mov 1, %t2
    tret