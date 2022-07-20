.origin 0x8000_0000
_start:
    mov 0x100, %xreg.ind
    mov 0xF80, %xreg.val
    mov $stack_top, %sp
    ;;;;; framebuffer_clear(12);
    mov 12, %a0
    bal %ra, @framebuffer_clear
    ;;;;; console_draw_window();
    bal %ra, @console_draw_window
    ;;;; console_puts(&banner);
    mov $banner, %a0
    bal %ra, @console_puts
    b @halt
halt:
    b @halt

.include "video.S"
.include "console.S"

.align 8
stack_bottom:
.filling 0,1024
stack_top:

banner: .string "LibreFirmware, Arch: IceWolf7800, Machine: WolfBox" .byte 0xA .string "(C) 2022 Talon396" .byte 0xA .byte 0x0