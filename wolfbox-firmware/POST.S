; uint64_t bank_test(bank_base -> a0);
bank_test:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push t0
    sub 8
    sd %t0
    ;;;;;;;;;;;;;;;;;;;
    ba %ra