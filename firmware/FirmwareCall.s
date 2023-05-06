/*
Firmware Calls:
    0x0100: ConsolePutChar (a0: Char | No Output)
    0x0101: ConsolePrint (a0: String | No Output)
    0x0102: ConsoleSetColor (a0: Color | No Output) (Obsolete, this now does nothing)
    0x0103: ConsoleGetChar (No Input | a1: Character)
    0x0104: ConsoleShow (a0: (1: Render Shade Only, 2: Render Console Only, 3: Render Shade and Console) | No Output) (Shade cannot be rendered anymore)
    0x01ff: FramebufferRenderBackground (No Input | No Output) (Obsolete, this now does nothing)
    0x0200: SCSISelect (a0: Physical SCSI ID | No Output)
    0x0201: SCSIRead (a0: LBA, a1: Blocks, a2: Address | No Output)
    0x0202: SCSIWrite (a0: LBA, a1: Blocks, a2: Address | No Output)
    0x0203: SCSIGetCapacity (No Input | a1: Blocks)
    0x0204: SCSIStartStop (a0: Start | No Output)
    0x0205: SCSIGetCondition (No Input | a1: KCQ)
    0x0300: OkamiBootGetRAMCapacity (No Input | a1: RAM Size (in bytes))
    0x0301: OkamiBootGetRevision (No Input | a1: Version)

Status Codes (a0):
    0: Success
    1: Unknown Failure
    2: SCSI Check Condition
    3: No Drive
    4: Invalid Call
    5: Invalid Argument
*/

.global DoFirmwareCall:
    addi sp, sp, -8
    sw ra, 4(sp)
    sw t0, 8(sp)
    mfex kr, 3
    andi kr, kr, 0xff00
    srli kr, kr, 8
    li t0, 1
    beq kr, t0, .section1
    li t0, 2
    beq kr, t0, .section2
    li t0, 3
    beq kr, t0, .section3
    b .invalid
.section1:
    mfex kr, 3
    andi kr, kr, 0xff
    bne kr, zero, 2
    bl ConsolePutc
    b .success
    li t0, 1
    bne kr, t0, 2
    bl ConsolePrint
    b .success
    li t0, 2
    bne kr, t0, 4
    /*la kr, consoleTextColor
    sw a0, 0(kr)*/
    nop
    nop
    nop
    b .success
    li t0, 3
    bne kr, t0, 3
    bl ConsoleGetChar
    mv a0, a1
    b .success
    li t0, 4
    bne kr, t0, 8
    mv a0, kr
    andi a0, a0, 1
    beq a0, zero, 1
    nop /*bl FramebufferDither*/
    andi kr, kr, 2
    beq kr, zero, 1
    bl ConsoleInit
    b .success
    li t0, 0xff
    bne kr, t0, .invalid
    /*bl FramebufferRenderBackground*/
    nop
    b .success
.section2:
    mfex kr, 3
    andi kr, kr, 0xff
    bne kr, zero, 2
    bl SCSISelect
    b .success
    li t0, 1
    bne kr, t0, 2
    bl SCSIReadBlocks
    b .ret
    li t0, 2
    bne kr, t0, 2
    bl SCSIWriteBlocks
    b .ret
    li t0, 3
    bne kr, t0, 7
    addi sp, sp, -4
    mv sp, a0
    addi a0, a0, 4
    bl SCSIGetCapacity
    lw a1, 4(sp)
    addi sp, sp, 4
    b .ret
    li t0, 4
    bne kr, t0, 2
    bl SCSIStartStop
    b .ret
    li t0, 5
    bne kr, t0, .invalid
    addi sp, sp, -8
    mv sp, a0
    mv sp, a1
    addi a1, a1, 4
    bl 0
    addi sp, sp, 8
    bne a0, zero, .ret
    lw a1, -8(sp)
    lw a2, -4(sp)
    b .success
.section3:
    mfex kr, 3
    andi kr, kr, 0xff
    li t0, 1
    bne kr, zero, 4
    la kr, RAMSize
    lw a1, 0(kr)
    b .success
    bne kr, t0, .invalid
    li a1, 0x0002 /* Revision 0.2 */
    beq zero, zero, .success
.invalid:
    li a0, 4
    beq zero, zero, .ret
.success:
    li a0, 0
.ret:
    lw ra, 4(sp)
    lw t0, 8(sp)
    addi sp, sp, 8
    rft
