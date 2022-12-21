.section .text.boot
.global _start
.align 4

_start:
	lui sp, 0x40
    // Copy Data Segment to Bottom of RAM
    li a0, 0x10
    la a1, __DATA_BEGIN__
    la a2, __DATA_END__
    jal ra, _mem_copy
    // Clear RAM for BSS Segment
    la a0, __BSS_BEGIN__
    la a1, __BSS_END__
    jal ra, _mem_clear
    // Run the EWFirmwareStart C Function
	addi sp,sp,-16
	sw	ra, 12(sp)
	jal	ra, EWFirmwareStart

_mem_copy:
// in:
//  a0 dest
//  a1 src
//  a2 end
// out: void
// clobbers: t0
    beq a1, a2, 1f
0:
    lw t0, 0(a1)
    addi a1, a1, 4
    sw t0, 0(a0)
    addi a0, a0, 4
    bltu a1, a2, 0b
1:
    jr ra
_mem_clear:
// in:
//  a0 dest
//  a1 end
// out: void
    beq a0, a1, 1f
0:
    sw zero, 0(a0)
    addi a0, a0, 4
    bltu a0, a1, 0b
1:
    jr ra
.section .header
.global _copyright_notice
_copyright_notice:
.string "EmberWolf Firmware, Copyright (C) 2022-2023 TalonFox.\nLicensed under the MIT License, see the LICENSE file for more information.\n\0"