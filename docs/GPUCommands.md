GPU Address: `0x20000000`
Layout:
```
0x2000_0000: String: 'EMBRGPU '
0x2000_0008:  u8   : Flags
        0b0000_0001: Texture Mapping Enable
        0b0000_0010: Lighting Enable
        0b0000_0100: Fog Enable
0x2000_0009-0x2000_000b: Reserved for data alignment
0x2000_000c:  u32  : Fog Color
0x2000_0010:  u32  : Fog Start
0x2000_0014:  u32  : Fog End
0x2000_0018:  u32  : Ambient Light Color
0x2000_001c-0x2000_007f: Reserved
0x2000_0080: Command FIFO

```
---
## ``