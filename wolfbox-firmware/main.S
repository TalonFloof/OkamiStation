.origin 0x8000_0000
_start:
    wrxreg 0x100, 0xF80
    mov $stack_top, %sp
    ;;;;; framebuffer_clear(0x004e98);
    as %sp
    sub 8
    sd %ra
    ;;;;;
    mov %a0, 12
    bal %ra, @framebuffer_clear
    ;;;;;
    as %sp
    ld %ra
    add 8
    ;;;;;
    b @halt
halt:
    b @halt

.include "video.S"

stack_bottom:
.filling 0,1024
stack_top:
