; void console_draw_window();
console_draw_window:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push a0, a1, a2, a3, a4, ra
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
    sd %ra
    ;;;;; framebuffer_rect(182,124,659,509,255);
    mov 182, %a0
    mov 124, %a1
    mov 659, %a2
    mov 509, %a3
    mov 255, %a4
    bal %ra, @framebuffer_rect
    ;;;;; framebuffer_rect(183,125,659,509,248);
    mov 183, %a0
    mov 125, %a1
    mov 659, %a2
    mov 509, %a3
    mov 248, %a4
    bal %ra, @framebuffer_rect
    ;;;;; framebuffer_rect(183,125,658,508,252);
    mov 183, %a0
    mov 125, %a1
    mov 658, %a2
    mov 508, %a3
    mov 252, %a4
    bal %ra, @framebuffer_rect

    ;;;;; framebuffer_rect(191,143,640,1,248);
    mov 191, %a0
    mov 143, %a1
    mov 641, %a2
    mov 1, %a3
    mov 248, %a4
    bal %ra, @framebuffer_rect
    ;;;;; framebuffer_rect(191,143,1,481,248);
    mov 191, %a0
    mov 143, %a1
    mov 1, %a2
    mov 481, %a3
    mov 248, %a4
    bal %ra, @framebuffer_rect

    ;;;;; framebuffer_rect(832,143,1,481,255);
    mov 832, %a0
    mov 143, %a1
    mov 1, %a2
    mov 481, %a3
    mov 255, %a4
    bal %ra, @framebuffer_rect
    ;;;;; framebuffer_rect(191,624,642,1,255);
    mov 191, %a0
    mov 624, %a1
    mov 642, %a2
    mov 1, %a3
    mov 255, %a4
    bal %ra, @framebuffer_rect

    ;;;;; framebuffer_rect(192,144,640,480,46);
    mov 192, %a0
    mov 144, %a1
    mov 640, %a2
    mov 480, %a3
    mov 46, %a4
    bal %ra, @framebuffer_rect
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; pop ra, a4, a3, a2, a1, a0
    ld %ra
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

; void console_putc(c -> a0);
console_putc:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push ra, a0, a1, a2, a3, a4, t0, t1, t2, t3
    sub 8
    sd %ra
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
    sd %t0
    sub 8
    sd %t1
    sub 8
    sd %t2
    sub 8
    sd %t3
    ;;;;;;;;;;;;;;;;;;;
    as %t2
    mov $cursor_x, %t2
    lb %t0
    mov $cursor_y, %t2
    lb %t1
    
    mov %a0, %t3
    mov %t0, %a0
    mov %t1, %a1
    as %a0
    mul 8
    add 192
    as %a1
    mul 16
    add 144
    mov 8, %a2
    mov 16, %a3
    mov 46, %a4
    bal %ra, @framebuffer_rect

    mov ' ', %t2
    bltu %t3, %t2, @console_putc.noprint
    mov %t3, %a2
    mov 255, %a3
    bal %ra, @framebuffer_draw_char
console_putc.noprint:
    mov 0x8, %t2
    beq %t3, %t2, @console_putc.backspace
    as %t0
    add 1
    mov 0xa, %t2
    beq %t3, %t2, @console_putc.newline
    mov 80, %t2
    bltu %t0, %t2, @console_putc.after_wrap
console_putc.newline:
    mov 0, %t0
    as %t1
    add 1
    mov 30, %t2
    bltu %t1, %t2, @console_putc.after_wrap

    bal %ra, @console_scroll
    mov 192, %a0
    mov 608, %a1
    mov 640, %a2
    mov 16, %a3
    mov 46, %a4
    bal %ra, @framebuffer_rect

    b @console_putc.after_wrap
console_putc.backspace:
    as %t0
    sub 1
    bge %t0, %zero, @console_putc.after_wrap
    mov 0, %t0
console_putc.after_wrap:
    mov %t0, %a0
    mov %t1, %a1
    as %a0
    mul 8
    add 192
    as %a1
    mul 16
    add 144
    mov 8, %a2
    mov 16, %a3
    mov 255, %a4
    bal %ra, @framebuffer_rect

    as %t2
    mov $cursor_x, %t2
    sb %t0
    mov $cursor_y, %t2
    sb %t1
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; pop t3, t2, t1, t0, a4, a3, a2, a1, a0, ra
    ld %t3
    add 8
    ld %t2
    add 8
    ld %t1
    add 8
    ld %t0
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
    ld %ra
    add 8
    ;;;;;;;;;;;;;;;;;;;
    ba %ra

; void console_puts(&s -> a0);
console_puts:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push ra, a0, a1, t0
    sub 8
    sd %ra
    sub 8
    sd %a0
    sub 8
    sd %a1
    sub 8
    sd %t0
    ;;;;;;;;;;;;;;;;;;;
    mov %a0, %a1
console_puts.loop:
    as %a1
    lb %t0
    add 1
    beq %t0, %zero, @console_puts.end
    mov %t0, %a0
    bal %ra, @console_putc
    b @console_puts.loop
console_puts.end:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; pop t0, a1, a0, ra
    ld %t0
    add 8
    ld %a1
    add 8
    ld %a0
    add 8
    ld %ra
    add 8
    ;;;;;;;;;;;;;;;;;;;
    ba %ra

console_scroll:
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; push ra, a0, a1, a2, t0, t1, t2
    sub 8
    sd %ra
    sub 8
    sd %a0
    sub 8
    sd %a1
    sub 8
    sd %a2
    sub 8
    sd %t0
    sub 8
    sd %t1
    sub 8
    sd %t2
    ;;;;;;;;;;;;;;;;;;;
    mov 0x780001000, %t0
    as %t0
    add 0x240C0
    mov 0, %t1
console_scroll.loop:
    mov %t0, %a0
    mov %t0, %a1
    as %a1
    add 16384
    mov 640, %a2
    bal %ra, @memcpy
    as %t0
    add 1024
    as %t1
    add 1
    mov 464, %t2
    bltu %t1, %t2, @console_scroll.loop
    ;;;;;;;;;;;;;;;;;;;
    as %sp
    ;;;;; pop t2, t1, t0, a2, a1, a0, ra
    ld %t2
    add 8
    ld %t1
    add 8
    ld %t0
    add 8
    ld %a2
    add 8
    ld %a1
    add 8
    ld %a0
    add 8
    ld %ra
    add 8
    ;;;;;;;;;;;;;;;;;;;
    ba %ra

cursor_x: .byte 0
cursor_y: .byte 0
.align 4