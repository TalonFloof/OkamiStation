# OKRO (OKami Relocatable Object)

## Header Info
```
0x0000_0000-0x0000_0007: Header (Encodes the string: "\x89OkamiRO")
0x0000_0008-0x0000_000f: Header Version (Set to 1)
0x0000_0010-0x0000_0013: Size of Text Segment
0x0000_0014-0x0000_0017: Size of Read-Only Data Segment
0x0000_0018-0x0000_001b: Size of Data Segment
0x0000_001c-0x0000_001f: Size of BSS Segment
0x0000_0020-0x0000_0023: Size of Relocation Table
0x0000_0024-0x0000_0027: Size of Symbol Table
0x0000_0028-0x0000_002b: Size of String Table (Symbol Names)
0x0000_002c-0x0000_002f: Offset of Entry Point within Text Segment
0x0000_0030: Beginning of Data (Starts with Text Segment)
```

> Note: All Segments are 4 byte aligned, however the size of the segment is exclusive of the extra alignment padding added.  

## Relocation Table Entry
```c
typedef enum {
    BRANCH28,
    PTR32,
    LA32
} RelocationType;

typedef enum {
    TEXT,
    RODATA,
    DATA,
    BSS,
    EXTERN, /* For an external symbol */
} SegmentType;

typedef struct {
    RelocationType type;
    SegmentType srcSegment;
    SegmentType dstSegment;
    uint32_t srcOffset;
    uint32_t dstOffset;
} RelocationEntry;
```

## Symbol Table Entry
```c
typedef enum {
    TEXT,
    RODATA,
    DATA,
    BSS,
    EXTERN, /* For an external symbol */
} SegmentType;

typedef struct {
    uint32_t nameoff;
    SegmentType segment;
    uint32_t offset;
    uint8_t external : 1;
    uint8_t libname : 1; /* For dynamic linking */
    uint8_t reserved : 6;
} Symbol;
```
