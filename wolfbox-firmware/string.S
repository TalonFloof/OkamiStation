; void memcpy(dest -> a0, src -> a1, count -> a2);
memcpy:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push a0, a1, a2, t0
    sub 8
    sd %a0
    sub 8
    sd %a1
    sub 8
    sd %a2
    sub 8
    sd %t0
    ;;;;;;;;;;;;;;;;;;;
memcpy.loop:
    as %a1
    lb %t0
    add 1
    as %a0
    sb %t0
    add 1

    as %a2
    sub 1
    bne %a2, %zero, @memcpy.loop
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;; pop t0, a2, a1, a0
    ld %t0
    add 8
    ld %a2
    add 8
    ld %a1
    add 8
    ld %a0
    add 8
    ;;;;;;;;;;;;;;;;;;;
    ba %ra