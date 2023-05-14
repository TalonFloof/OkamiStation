.text
.global OkamiStationFirmwareStartup:
    /*
    At the current CPU State, there are a few things which are in an invalid state.
    For Instance:
      - The L1 Instruction and Data Caches are in an unknown state.
      - The TLB is in an unknown state.
    Most CPUs would handle this stuff in hardware when the reset pin is high,
    but the Okami processor does not, it requires software to do this.

    You may be wondering how we are able to execute code if the L1 Caches and the TLB isn't setup yet.
    The answer is that the address ranges 0xa0000000-0xbfffffff (known as kernel2)
    doesn't use the L1 Caches or the TLB, it allows you to access the first 512 MiB of physical memory
    directly (which bypasses the need for the L1 Caches) if you are in kernel mode, which we are at reset.
    This is why we can execute code, even with this stuff not working.
    Addresses 0x80000000-0x9fffffff (known as kernel1) is the same as kernel2 but it uses the L1 Caches.
    */
    la a0, EarlyHandler
    lui t1, 0x2000
    add a0, a0, t1
    mtex a0, 5
    /* We must clear the caches */
    bl ClearICache
    bl ClearDCache
    /* Clear the framebuffer */
    lui a0, 0xb000
    sw zero, 0(a0)
    addi a0, a0, 0x1000
    la a1, 786432
    mv zero, a2
    bl memset
    /* Clear the TLB next */
    bl ClearTLB
    /* Setup Stack */
    la sp, FWstack
    addi sp, sp, 0xffc
    /* Clear the BSS segment */
    la a0, 0xa0000000
    la a1, __BSS_END__
    lui t0, 0x8000
    sub a1, a1, t0
    mv zero, a2
    bl memset
    la t0, main
    br t0
halt:
    beq zero, zero, halt

.global ClearICache:
    lui t1, 0x8000
    la t2, 0x80004000
    mfex t0, 0x00 /* OKAMI_STATUS */
    ori t0, t0, 0xc0
    mtex t0, 0x00 /* OKAMI_STATUS */
.icache_loop:
    sw zero, 0(t1)
    sw zero, 4(t1)
    addi t1, t1, 8
    bltu t1, t2, .icache_loop
    andi t0, t0, 0xff3f
    mtex t0, 0x00 /* OKAMI_STATUS */
    br ra

.global ClearDCache:
    lui t1, 0x8000
    la t2, 0x80004000
    mfex t0, 0x00 /* OKAMI_STATUS */
    ori t0, t0, 0x40
    mtex t0, 0x00 /* OKAMI_STATUS */
.dcache_loop:
    sw zero, 0(t1)
    sw zero, 4(t1)
    addi t1, t1, 8
    bltu t1, t2, .dcache_loop
    andi t0, t0, 0xff3f
    mtex t0, 0x00 /* OKAMI_STATUS */
    br ra

.global ClearTLB:
    mv zero, t0
    li t1, 64
    mtex zero, 0x11 /* OKAMI_TLB_VALUE_LOW */
    mtex zero, 0x12 /* OKAMI_TLB_VALUE_HIGH */
.loop:
    mtex t0, 0x10 /* OKAMI_TLB_INDEX */
    tlbwi
    addi t0, t0, 1
    bltu t0, t1, .loop
    br ra

.global Jump:
    la sp, 0x80000ffc
    la t0, ClearICache
    lui t1, 0x2000
    add t0, t0, t1
    mv a0, ra
    br t0
    b halt /* This should never happen, but just in case */

.bss
FWstack: .resb 4096
.text
