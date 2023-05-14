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
    addi sp, sp, -4
    sw ra, 4(sp)
    mfex k0, 3
    andi k0, k0, 0xff00
    srli k0, k0, 8
    li k1, 1
    beq k0, k1, .section1
    li k1, 2
    beq k0, k1, .section2
    li k1, 3
    beq k0, k1, .section3
    b .invalid
.section1:
    mfex k0, 3
    andi k0, k0, 0xff
    bne k0, zero, 2
    bl ConsolePutc
    b .success
    li k1, 1
    bne k0, k1, 2
    bl ConsolePrint
    b .success
    li k1, 2
    bne k0, k1, 4
    /*la k0, consoleTextColor
    sw a0, 0(k0)*/
    nop
    nop
    nop
    b .success
    li k1, 3
    bne k0, k1, 3
    bl ConsoleGetChar
    mv a0, a1
    b .success
    li k1, 4
    bne k0, k1, 8
    mv a0, k0
    andi a0, a0, 1
    beq a0, zero, 1
    nop /*bl FramebufferDither*/
    andi k0, k0, 2
    beq k0, zero, 1
    bl ConsoleInit
    b .success
    li k1, 0xff
    bne k0, k1, .invalid
    /*bl FramebufferRenderBackground*/
    nop
    b .success
.section2:
    mfex k0, 3
    andi k0, k0, 0xff
    bne k0, zero, 2
    bl SCSISelect
    b .success
    li k1, 1
    bne k0, k1, 2
    bl SCSIReadBlocks
    b .ret
    li k1, 2
    bne k0, k1, 2
    bl SCSIWriteBlocks
    b .ret
    li k1, 3
    bne k0, k1, 7
    addi sp, sp, -4
    mv sp, a0
    addi a0, a0, 4
    bl SCSIGetCapacity
    lw a1, 4(sp)
    addi sp, sp, 4
    b .ret
    li k1, 4
    bne k0, k1, 2
    bl SCSIStartStop
    b .ret
    li k1, 5
    bne k0, k1, .invalid
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
    mfex k0, 3
    andi k0, k0, 0xff
    li k1, 1
    bne k0, zero, 4
    la k0, RAMSize
    lw a1, 0(k0)
    b .success
    bne k0, k1, .invalid
    li a1, 0x0002 /* Revision 0.2 */
    beq zero, zero, .success
.invalid:
    li a0, 4
    beq zero, zero, .ret
.success:
    li a0, 0
.ret:
    lw ra, 4(sp)
    addi sp, sp, 4
    rft
