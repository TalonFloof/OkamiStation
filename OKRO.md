# OKRO (OKami Relocatable Object)

## Header Info
```
0x0000_0000-0x0000_0007: Header (Encodes the string: "\x89OkamiRO")
0x0000_0008-0x0000_000f: Header Version (Set to 1)
0x0000_0010-0x0000_0017: Size of Text and Read-Only Data Segment
0x0000_0018-0x0000_001f: Size of Data Segment
0x0000_0020-0x0000_0027: Size of BSS Segment
0x0000_0028-0x0000_002f: Size of Relocation Table
0x0000_0030-0x0000_0037: Size of Extended Information Table
0x0000_0038-0x0000_00ff: Reserved
0x0000_0100: Beginning of Data (Starts with Text and Read-Only Data Segment)
```

## Relocation Table Entry
```c
typedef enum {
    BRANCH28,
    PTR16,
    PTR32,
    LA32,
    AUPC16
} RelocationType;

typedef struct {
    RelocationType type;
    uint64_t offset;
} RelocationEntry;
```
