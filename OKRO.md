# OKRO (OKami Relocatable Object)

## Header Info
```
0x0000_0000-0x0000_0007: Header (Encodes the string: "\x89OkamiRO")
0x0000_0008-0x0000_000f: Header Version
0x0000_0010-0x0000_0017: Size of Text and Read-Only Data Segment
0x0000_0018-0x0000_001f: Size of Data Segment
0x0000_0020-0x0000_0027: Size of BSS Segment
0x0000_0028-0x0000_002f: Size of Relocation Segment
```