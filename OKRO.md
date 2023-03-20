# OKRO (OKami Relocatable Object)

## Header Info
```c
typedef struct {
    unsigned char magic[8]; /* "\x89OkamiRO" */
    unsigned int version; /* Set to 1 */
    unsigned int text; /* Size of Text Segment */
    unsigned int rodata; /* Size of Read-Only Segment */
    unsigned int data; /* Size of Data Segment */
    unsigned int bss; /* Size of BSS Segment */
    unsigned int reloc; /* Size of Relocation Table */
    unsigned int sym; /* Size of Symbol Table */
    unsigned int str; /* Size of String Table */
    unsigned int roBase; /* Base Address of Text and Read-Only segment (only set if object has been relocated) */
    unsigned int rwBase; /* Base Address of Data and BSS segment (only set if object has been relocated) */
    unsigned int entry; /* Entrypoint Offset (within Text Segment) */
} OkROHeader;
```

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
