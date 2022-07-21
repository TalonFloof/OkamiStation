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
    sd %t2
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
    ld %zero
    fence 2
    bne %t2, %zero, @bank_test.end
    add 16
    as %t0
    add 16
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

; uint64_t memory_test();
memory_test:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push ra, t0, t1, t2
    sub 8
    sd %ra
    sub 8
    sd %t0
    sub 8
    sd %t1
    sub 8
    sd %t2
    ;;;;;;;;;;;;;;;;;;;
    mov 0, %t0
    mov 8, %t1
    mov 0, %t2
memory_test.loop:
    mov %t0, %a0
    bal %ra, @bank_test
    as %t2
    add %a0
    as %t0
    add 0x4000000
    as %t1
    sub 1
    bne %t1, %zero, @memory_test.loop
    mov %t2, %a0
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; pop t2, t1, t0, ra
    ld %t2
    add 8
    ld %t1
    add 8
    ld %t0
    add 8
    ld %ra
    add 8
    ;;;;;;;;;;;;;;;;;;;
    ba %ra