.origin 0x8000_0000
_start:
    mov 0x100, %xreg.ind
    mov 0x24F80, %xreg.val
    mov $stack_top, %sp
    ;;;;; framebuffer_clear(12);
    mov 12, %a0
    bal %ra, @framebuffer_clear
    ;;;;; console_draw_window();
    bal %ra, @console_draw_window
    ;;;; console_puts(&banner);
    mov $banner, %a0
    bal %ra, @console_puts
    
    mov $memory_test_msg, %a0
    bal %ra, @console_puts
    bal %ra, @memory_test
    as %a0
    divu 1024
    bal %ra, @console_putint
    mov $memory_suffix, %a0
    bal %ra, @console_puts

    b @halt
halt:
    b @halt

.include "video.S"
.include "console.S"
.include "POST.S"
.include "string.S"

.align 8
stack_bottom:
.filling 0,1024
stack_top:

banner: .string "LibreFirmware, Arch: IceWolf7800, Machine: WolfBox" .byte 0xA .string "(C) 2022 Talon396" .byte 0xA .byte 0x0
memory_test_msg: .string "Testing Random-Access Memory... " .byte 0x0
memory_suffix: .string " KiB OK" .byte 0xA .byte 0x0