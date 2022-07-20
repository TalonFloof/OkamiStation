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