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

; void framebuffer_rect(x -> a0, y -> a1, w -> a2, h -> a3, color -> a4);
; All registers are preserved
framebuffer_rect:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push a0, a1, a2, a3, a4, a5, a6, a7
    sub 8
    sd %a0
    sub 8
    sd %a1
    sub 8
    sd %a2
    sub 8
    sd %a3
    sub 8
    sd %a4
    sub 8
    sd %a5
    sub 8
    sd %a6
    sub 8
    sd %a7
    ;;;;;;;;;;;;;;;;;;;
    mov 0x780001000, %a5
    mov 1024, %a6

    as %a1 ; y * width
    mul %a6
    as %a0 ; y * width + x
    add %a1
    as %a5
    add %a0
    mov %a5, %a0

    mov %a6, %a7
    mov %a2, %a6

framebuffer_rect.y_loop:
    mov %a2, %a5
framebuffer_rect.x_loop:
    as %a0
    sb %a4
    add 1
    as %a5
    sub 1
    bne %a5, %zero, @framebuffer_rect.x_loop

    as %a0
    sub %a6
    add %a7
    as %a3
    sub 1
    bne %a3, %zero, @framebuffer_rect.y_loop
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; pop a7, a6, a5, a4, a3, a2, a1, a0
    ld %a7
    add 8
    ld %a6
    add 8
    ld %a5
    add 8
    ld %a4
    add 8
    ld %a3
    add 8
    ld %a2
    add 8
    ld %a1
    add 8
    ld %a0
    add 8
    ;;;;;;;;;;;;;;;;;;;
    ba %ra

; void framebuffer_draw_char(x -> a0, y -> a1, char -> a2, color -> a3);
framebuffer_draw_char:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push a0, a1, a2, a4, a5, a6, a7, t0
    sub 8
    sd %a0
    sub 8
    sd %a1
    sub 8
    sd %a2
    sub 8
    sd %a4
    sub 8
    sd %a5
    sub 8
    sd %a6
    sub 8
    sd %a7
    sub 8
    sd %t0
    ;;;;;;;;;;;;;;;;;;;
    as %a2
    mul 16
    add $framebuffer_charset

    mov 0x780001000, %a4
    mov 0, %a6
    mov %a1, %a7
    as %a7
    mul 1024
    add %a0
    as %a4
    add %a7

framebuffer_draw_char.y_loop:
    mov 0b1000_0000, %a5
    as %a2
    lb %a7
framebuffer_draw_char.x_loop:
    mov %a7, %t0
    as %t0
    and %a5
    beq %t0, %zero, @framebuffer_draw_char.no_draw
    as %a4
    sb %a3
framebuffer_draw_char.no_draw:
    ;;;;;
    as %a5
    srl 1
    as %a4
    add 1
    bne %a5, %zero, @framebuffer_draw_char.x_loop

    as %a2
    add 1
    as %a4
    sub 8
    add 1024
    as %a6
    add 1
    mov 16, %t0
    bltu %a6, %t0, @framebuffer_draw_char.y_loop

    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; pop t0, a7, a6, a5, a4, a2, a1, a0
    ld %t0
    add 8
    ld %a7
    add 8
    ld %a6
    add 8
    ld %a5
    add 8
    ld %a4
    add 8
    ld %a2
    add 8
    ld %a1
    add 8
    ld %a0
    add 8
    ;;;;;;;;;;;;;;;;;;;
    ba %ra

framebuffer_charset:
.binary_include "charset.bin"