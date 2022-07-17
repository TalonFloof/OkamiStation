.origin 0x8000_0000
_start:
    wrxreg 0x100, 0xF80
    mov $stack_top, %sp
    ;;;;; framebuffer_clear(0x004e98);
    mov %a0, 12
    bal %ra, @framebuffer_clear
    b @halt
halt:
    b @halt

.include "video.S"

stack_bottom:
.filling 0,1024
stack_top:
