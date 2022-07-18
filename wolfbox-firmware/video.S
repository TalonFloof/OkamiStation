; void framebuffer_clear(color -> a0);
; All registers are preserved
framebuffer_clear:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push t0, t1
    sub 8
    sd %t0
    sub 8
    sd %t1
    ;;;;;;;;;;;;;;;;;;;
    mov 0x780001000, %t0 ; Pointer to the framebuffer on WolfBox
    mov 786432, %t1
framebuffer_clear.loop:
    as %t0
    sb %a0
    add 1
    as %t1
    sub 1
    bne %t1, %zero, @framebuffer_clear.loop
framebuffer_clear.after:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; pop t1, t0
    ld %t1
    add 8
    ld %t0
    add 8
    ;;;;;;;;;;;;;;;;;;;
    ba %ra