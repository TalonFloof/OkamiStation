# Instructions
## Arithmetic
```
    add rd,rs1,rs2
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b000000

        rd[0:31] = rs1[0:31] + rs2[0:31];
    sub rd,rs1,rs2
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b000001
        
        rd[0:31] = rs1[0:31] - rs2[0:31];
    and rd,rs1,rs2
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b000010
        
        rd[0:31] = rs1[0:31] & rs2[0:31];
    or rd,rs1,rs2
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b000011
        
        rd[0:31] = rs1[0:31] | rs2[0:31];
    xor rd,rs1,rs2
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b000100
        
        rd[0:31] = rs1[0:31] ^ rs2[0:31];
    sll rd,rs1,rs2
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b000101
       
        rd[0:31] = rs1[0:31] << rs2[0:31];
    srl rd,rs1,rs2
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b000110
       
        rd[0:31] = rs1[0:31] >>> rs2[0:31];
    sra rd,rs1,rs2
        [7:10]: 8
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b000110
       
        rd[0:31] = rs1[0:31] >> rs2[0:31];
    slt rd,rs1,rs2
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b000111
        
        rd[0:31] = (rs1[0:31] < rs2[0:31]) ? 1 : 0;
    sltu rd,rs1,rs2
        [11:15]: rd
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b001000
        
        rd[0:31] = (rs1[0:31] < rs2[0:31]) ? 1 : 0;
    mul rd, ru, rs1, rs2
        [6:10]: rd
        [11:15]: ru
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b001001
        
        rd[0:31] = (rs1[0:31] * rs2[0:31]) & 0xffffffff;
        ru[0:31] = (rs1[0:31] * rs2[0:31]) >>> 32;
    mulu rd,ru,rs1,rs2
        [0:3]: 1
        [6:10]: rd
        [11:15]: ru
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b001001
        
        rd[0:31] = (rs1[0:31] * rs2[0:31]) & 0xffffffff;
        ru[0:31] = (rs1[0:31] * rs2[0:31]) >>> 32;
    div rd,ru,rs1,rs2
        [6:10]: rd
        [11:15]: ru
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b001010
        
        rd[0:31] = rs1[0:31] / rs2[0:31];
        ru[0:31] = rs1[0:31] % rs2[0:31];
    divu rd,ru,rs1,rs2
        [0:3]: 1
        [6:10]: rd
        [11:15]: ru
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b001010
        
        rd[0:31] = rs1[0:31] / rs2[0:31];
        ru[0:31] = rs1[0:31] % rs2[0:31];
```
## Arithmetic w/ Immediate
```
    addi rd,rs,const
        [0:15]: const
        [16:20]: rd
        [21:25]: rs
        [26:31]: 0b010000
        
        rd[0:31] = rs[0:31] + const;
    andi rd,rs,const
        [0:15]: const
        [16:20]: rd
        [21:25]: rs
        [26:31]: 0b010001
        
        rd[0:31] = rs[0:31] & const;
    ori rd,rs,const
        [0:15]: const
        [16:20]: rd
        [21:25]: rs
        [26:31]: 0b010010
       
        rd[0:31] = rs[0:31] | const;
    xori rd,rs,const
        [0:15]: const
        [16:20]: rd
        [21:25]: rs
        [26:31]: 0b010011
       
        rd[0:31] = rs[0:31] ^ const;
    slli rd,rs,const
        [0:4]: const
        [5:15]: 0
        [16:20]: rd
        [21:25]: rs
        [26:31]: 0b010100
       
        rd[0:31] = rs[0:31] << const;
    srli rd,rs,const
        [0:4]: const
        [5:15]: 0
        [16:20]: rd
        [21:25]: rs
        [26:31]: 0b010101
       
        rd[0:31] = rs[0:31] >>> const;
    srai rd,rs,const
        [0:4]: const
        [5:11]: 0
        [12:15]: 8
        [16:20]: rd
        [21:25]: rs
        [26:31]: 0b010101
       
        rd[0:31] = rs[0:31] >> const;
    slti rd,rs,const
        [0:15]: const
        [16:20]: rd
        [21:25]: rs
        [26:31]: 0b010110
        
        rd[0:31] = (rs[0:31] < const) ? 1 : 0;
    sltiu rd,rs,const
        [0:15]: const
        [16:20]: rd
        [21:25]: rs
        [26:31]: 0b010111
        
        rd[0:31] = (rs[0:31] < const) ? 1 : 0;
    lui rd,const
        [0:15]: const
        [16:20]: rd
        [26:31]: 0b011000
        
        rd[0:31] = const << 16
    aupc const
        [0:15]: const
        [16:20]: rd
        [26:31] 0b011001
        
        rd[0:31] = PC + (const << 16);
```
## Branching
```
    bl rd, offset
        [0:20]: offset
        [21:25]: rd
        [26:31] 0b100000
    
        rd[0:31] = PC+4;
        PC[0:31] += offset << 2;
    blr rd, rs
        [16:20]: rs
        [21:25]: rd
        [26:31] 0b100001
    
        rd[0:31] = PC+4;
        PC[0:31] = rs[0:31];
    beq rs1, rs2, offset
        [0:15]: offset
        [16:20]: rs1
        [21:25]: rs2
        [26:31] 0b100010
        
        if(rs1[0:31] == rs2[0:31]) {
            PC[0:31] += offset << 2;
        }
    bne rs1, rs2, offset
        [0:15]: offset
        [16:20]: rs1
        [21:25]: rs2
        [26:31] 0b100011
        
        if(rs1[0:31] != rs2[0:31]) {
            PC[0:31] += offset << 2;
        }
    bge rs1, rs2, offset
        [0:15]: offset
        [16:20]: rs1
        [21:25]: rs2
        [26:31] 0b100100
        
        if(rs1[0:31] >= rs2[0:31]) {
            PC[0:31] += offset << 2;
        }
    blt rs1, rs2, offset
        [0:15]: offset
        [16:20]: rs1
        [21:25]: rs2
        [26:31] 0b100101
        
        if(rs1[0:31] < rs2[0:31]) {
            PC[0:31] += offset << 2;
        }
    bgeu rs1, rs2, offset
        [0:15]: offset
        [16:20]: rs1
        [21:25]: rs2
        [26:31] 0b100110
        
        if(rs1[0:31] >= rs2[0:31]) {
            PC[0:31] += offset << 2;
        }
    bltu rs1, rs2, offset
        [0:15]: offset
        [16:20]: rs1
        [21:25]: rs2
        [26:31]: 0b100111
        
        if(rs1[0:31] < rs2[0:31]) {
            PC[0:31] += offset << 2;
        }
```
## Memory
```
    lb rd, const(rs)
        [0:15]: offset
        [16:20]: rs
        [21:25]: rd
        [26:31]: 0b110000
    lbu rd, const(rs)
        [0:15]: offset
        [16:20]: rs
        [21:25]: rd
        [26:31]: 0b110001
    lh rd, const(rs)
        [0:15]: offset
        [16:20]: rs
        [21:25]: rd
        [26:31]: 0b110010
    lhb rd, const(rs)
        [0:15]: offset
        [16:20]: rs
        [21:25]: rd
        [26:31]: 0b110011
    lw rd, const(rs)
        [0:15]: offset
        [16:20]: rs
        [21:25]: rd
        [26:31]: 0b110100
    sb rd, const(rs)
        [0:15]: offset
        [16:20]: rs
        [21:25]: rd
        [26:31]: 0b110101
    sh rd, const(rs)
        [0:15]: offset
        [16:20]: rs
        [21:25]: rd
        [26:31]: 0b110110
    sw rd, const(rs)
        [0:15]: offset
        [16:20]: rs
        [21:25]: rd
        [26:31]: 0b110111
```
## Extended Registers
```
    mfex rd, regnum
        [0:15]: offset
        [21:25]: rd
        [26:31]: 0b111100
        
        Move Extended Register to Register
    mtex rs, regnum
        [0:15]: offset
        [16:20]: rs
        [26:31]: 0b111101
        
        Move Register Value to Extended Register
```

## Coprocessor
```
    coproc
        [0:25] Coprocessor Instruction and Arguments
        [26:31]: 0b001111
        
        Only available if a coprocessor is externally installed.
        This can be used for calculating floating-point numbers.
```

## Traps
```
    kcall code
        [0:25] Code (can be any value)
        [26:31] 0b111110
        
        Triggers KCALL Trap
    rft
        [26:31] 0b111111
        
        OKAMI_STATUS = ((OKAMI_STATUS & 3) >> 1) | (OKAMI_STATUS & ~3);
        PC = OAMKI_TRAP_PC;
```

# Memory Map
```
0x00000000-0x3fffffff user (MMU Enabled, Caches Enabled)
0x40000000-0x7fffffff kernel1 (MMU Enabled, Caches Enabled)
0x80000000-0xbfffffff kernel2 (MMU Disabled, Caches Enabled)
0xc0000000-0xffffffff kernel3 (MMU Disabled, Caches Disabled)
```

# ABI
```
r0 - zero: Hardwired Zero
r1-r8 - a0-a7: Arguments/Return Values
r9-r16 - t0-t7: Temporary/Scratchpad Values
r17-r26 - s0-s9: Saved Values
r27 - gp: Global Pointer
r28 - tp: Thread Pointer (for Local Thread Storage stuff)
r29 - fp: Frame Pointer
r30 - sp: Stack Pointer
r31 - ra: Return Address
```

# Extended Registers
```
0x00: OKAMI_STATUS
    0x1: Current Mode (0: User, 1: Kernel. Set to 1 on reset)
    0x2: Previous Mode (Both bit 0 and 1 act as a 2-level stack for processor modes with traps)
    0x4: Enable External Trap (Toggles the External Trap which allows for external interrupts)
       NOTE: This is set to zero when a trap is triggered to prevent nesting (it can be reenabled though)
    0x8: Cache Isolate
       NOTE: This acts similar to how MIPS-1 does it. It allows the CPU to directly access the L1 Data Cache using the kernel1 segment.
    0x10: Cache Swap
       NOTE: This acts similar to how MIPS-1 does it. It swaps the L1 Data and L1 Instruction Caches when isolating them.
    
0x01: OKAMI_TRAP_CAUSE
    0: N/A
    1: External Trap
    2: KCall
    3: TLB Miss (Triggers separate vector for this)
    4: Unknown Opcode
    6: Misaligned Read
    7: Misaligned Write
    8: Fetch Exception
    9: Data Exception
0x02: OKAMI_TRAP_PC
0x03: OKAMI_TRAP_BAD_VADDR
0x04: OAMKI_TRAP_KERNEL_SCRATCH
0x05: OKAMI_TRAP_VECTOR_OFFSET
    NOTE: DO NOT ENABLE EXTERNAL TRAPS UNTIL THIS HAS BEEN SET!
0x10: OKAMI_TLB_INDEX
0x11: OKAMI_TLB_VALUE_LOW
0x12: OAMKI_TLB_VALUE_HIGH
0x13: OKAMI_TLB_RANDOM_INDEX
    Starts from 63 and decrements every cpu tick, it wraps around after trying to decrement 0.
    Useful if you just want to fill a random TLB entry.
```

# Cache Line Layout
| <sub>63-60</sub><br>Parity | <sub>59</sub><br>Valid | <sub>58-56</sub><br>Reserved | <sub>55-32</sub><br>HighAddress | <sub>0-31</sub><br>Word |
|-|-|-|-|-|
> Note: Parity is computed by Exclusive ORing (XORing) every nibble before the parity nibble (including the reserved spot and the valid bit)
# TLB Line Layout
| <sub>63-40</sub><br>PhysicalAddress | <sub>39-32</sub><br>AddrSpaceID | <sub>8-31</sub><br>VirtualAddress | <sub>3-7</sub><br>Size (1&lt;&lt;n) | <sub>2</sub><br>Dirty | <sub>1</sub><br>NonCacheable | <sub>0</sub><br>Valid |
|-|-|-|-|-|-|-|
> Note: Size must be a least 8 (which is a 256 byte page), any value less than that will trigger a TLB miss when accessed.