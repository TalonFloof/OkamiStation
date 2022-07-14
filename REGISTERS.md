|      | +0x0     | +0x1     | +0x2     | +0x3     | +0x4     | +0x5     | +0x6     | +0x7     | +0x8     | +0x9     | +0xa     | +0xb     | +0xc     | +0xd      | +0xe     | +0xf      |
|------|----------|----------|----------|----------|----------|----------|----------|----------|----------|----------|----------|----------|----------|-----------|----------|-----------|
| 0x00 | pc       | ra       | sp       | gp       | tp       | t0       | t1       | t2       | s0       | s1       | a0       | a1       | a2       | a3        | a4       | a5        |
| 0x10 | a6       | a7       | s2       | s3       | s4       | s5       | s6       | s7       | s8       | s9       | s10      | s11      | t3       | t4        | t5       | t6        |
| 0x20 | alu.acc  | alu.add  | alu.sub  | alu.xor  | alu.or   | alu.and  | alu.sll  | alu.srl  | alu.sra  | alu.slt  | alu.sltu | alu.mul  | alu.mulh | alu.mulsu | alu.mulu | alu.div   |
| 0x30 | alu.divu | alu.rem  | alu.remu | atom.lrw | atom.scw | atom.swp | atom.add | atom.and | atom.or  | atom.xor | atom.max | atom.min |          |           |          | nop       |
| 0x40 | mem.lb   | mem.lh   | mem.lw   | mem.ld   | mem.sb   | mem.sh   | mem.sw   | mem.sd   |          |          |          |          |          |           |          | mem.fence |
| 0x50 | bran.eq  | bran.ne  | bran.lt  | bran.ge  | bran.ltu | bran.geu | bran.lnk | bran.rel | bran.rln |          | trapret  | ecall    | xreg.ind | xreg.val  | xreg.swp | lui       |
