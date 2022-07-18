.origin 0x8000_0000
_start:
    mov 0x100, %xreg.ind
    mov 0xF80, %xreg.val
    mov $stack_top, %sp
    ;;;;; framebuffer_clear(0x004e98);
    as %sp
    sub 8
    sd %ra
    ;;;;;
    mov 12, %a0
    bal %ra, @framebuffer_clear
    ;;;
    ld %ra
    add 8
    ;;;;;
    b @halt
halt:
    b @halt

.include "video.S"

stack_bottom:
.filling 0,256
stack_top:
