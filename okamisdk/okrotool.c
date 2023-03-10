#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    uint64_t magic;
    uint64_t version;
    uint32_t text;
    uint32_t rodata;
    uint32_t data;
    uint32_t bss;
    uint32_t reloc;
    uint32_t eit;
    uint64_t reserved;
} OkROHeader;

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
    EXTERN
} SegmentType;

typedef struct {
    RelocationType type;
    SegmentType srcSegment;
    SegmentType dstSegment;
    uint32_t srcOffset;
    uint32_t dstOffset;
} RelocationEntry;

const char* segToString(SegmentType type) {
    if(type == TEXT) {
        return ".text";
    } else if(type == RODATA) {
        return ".rodata";
    } else if(type == DATA) {
        return ".data";
    } else if(type == BSS) {
        return ".bss";
    }
    return "???";
}

uint32_t getSize(uint32_t num) {
    return num + ((num % 4 > 0) ? 4-(num % 4) : 0);
}

uint8_t* readImage(const char* path, size_t* imgSize) {
    FILE* file = fopen(path,"rb");
    if(file == NULL)
        return NULL;
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    if(imgSize != NULL) {
        *imgSize = fsize;
    }
    fseek(file, 0, SEEK_SET);
    uint8_t* content = malloc(fsize);
    if(fread(content, fsize, 1, file) != 1)
        return NULL;
    fclose(file);
    return content;
}

void writeImage(const char* path, uint8_t* base, size_t len) {
    FILE* file = fopen(path,"wb");
    if(file == NULL) {
        fprintf(stderr, "Couldn't open file for image writing!\n");
        fclose(file);
        exit(1);
    }
    if(fwrite(base,len,1,file) != 1) {
        fprintf(stderr, "Failed to write to file!\n");
        fclose(file);
        exit(1);
    }
    fclose(file);
}

void preformRelocation(uint8_t* image, uint32_t base, uint32_t dataBase) {
    OkROHeader* header = (OkROHeader*)image;
    RelocationEntry* relocation = (RelocationEntry*)(((uintptr_t)header)+sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data));
    uint32_t textAddr = base;
    uint32_t rodataAddr = textAddr+getSize(header->text);
    uint32_t dataAddr = (dataBase != 0) ? dataBase : (rodataAddr+getSize(header->rodata));
    uint32_t bssAddr = dataAddr+getSize(header->data);
    uint32_t entries = header->reloc/sizeof(RelocationEntry);
    for(int i=0; i < entries; i++) {
        uint32_t labelAddr;
        uint32_t* dstPtr;
        if(relocation[i].dstSegment == TEXT) {
            labelAddr = textAddr+relocation[i].dstOffset;
        } else if(relocation[i].dstSegment == RODATA) {
            labelAddr = rodataAddr+relocation[i].dstOffset;
        } else if(relocation[i].dstSegment == DATA) {
            labelAddr = dataAddr+relocation[i].dstOffset;
        } else if(relocation[i].dstSegment == BSS) {
            labelAddr = bssAddr+relocation[i].dstOffset;
        }
        if(relocation[i].srcSegment == TEXT) {
            dstPtr = (uint32_t*)((uintptr_t)header+(uintptr_t)((textAddr-base+sizeof(OkROHeader))+relocation[i].srcOffset));
        } else if(relocation[i].srcSegment == RODATA) {
            dstPtr = (uint32_t*)((uintptr_t)header+(uintptr_t)((rodataAddr-base+sizeof(OkROHeader))+relocation[i].srcOffset));
        } else if(relocation[i].srcSegment == DATA) {
            if(dataBase != 0) {
                dstPtr = (uint32_t*)((uintptr_t)header+(uintptr_t)((dataAddr-dataBase+sizeof(OkROHeader))+relocation[i].srcOffset));
            } else {
                dstPtr = (uint32_t*)((uintptr_t)header+(uintptr_t)((dataAddr-base+sizeof(OkROHeader))+relocation[i].srcOffset));
            }
        } else if(relocation[i].srcSegment == BSS) {
            if(dataBase != 0) {
                dstPtr = (uint32_t*)((uintptr_t)header+(uintptr_t)((bssAddr-dataBase+sizeof(OkROHeader))+relocation[i].srcOffset));
            } else {
                dstPtr = (uint32_t*)((uintptr_t)header+(uintptr_t)((bssAddr-base+sizeof(OkROHeader))+relocation[i].srcOffset));
            }
        }
        switch(relocation[i].type) {
            case BRANCH28: {
                uint32_t op = (*dstPtr) & 0xFC000000;
                *dstPtr = (((labelAddr >> 2) & 0x3FFFFFF) | op);
                break;
            }
            case PTR32:
                *dstPtr = labelAddr;
                break;
            case LA32:
                ((uint16_t*)dstPtr)[0] = (uint16_t)(labelAddr >> 16);\
                ((uint16_t*)dstPtr)[2] = (uint16_t)(labelAddr & 0xFFFF);
                break;
            default:
                fprintf(stderr, "Unknown relocation type: %i\n", relocation[i].type);
                exit(1);
                break;
        }
    }    
}

int main(int argc, const char* argv[]) {
    if(argc == 1) {
        printf("Usage: okrotool [command]\n");
        printf("Commands:\n");
        printf("info [object]: Get Info about a Object.\n");
        printf("dump [base] [object] [binary]: Convert an object into a raw binary.\n");
        printf("fwdump [base] [dataBase] [object] [binary]: Convert an object into an OkamiStation firmware binary.\n");
    } else if(strcmp(argv[1],"info") == 0) {
        uint8_t* image = readImage(argv[2],NULL);
        if(!image) {
            fprintf(stderr, "Unable to read image!\n");
            free(image);
            return 1;
        }
        OkROHeader* header = (OkROHeader*)image;
        if(memcmp(image,"\x89OkamiRO",8) != 0) {
            fprintf(stderr, "Magic number is invalid\n");
            free(image);
            return 2;
        }
        printf("== %s ==\nVersion: %i\nText Size: %i bytes (~%i instructions)\nRoData Size: %i bytes\nData Size: %i bytes\nBSS Size: %i bytes\nRelocation Entries: %i\n", argv[2], header->version, header->text, header->text/4, header->rodata, header->data, header->bss, header->reloc/20);

        printf("== RELOCATION ENTRIES ==\n");
        RelocationEntry* relocation = (RelocationEntry*)(((uintptr_t)header)+sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data));
        uint32_t entries = header->reloc/sizeof(RelocationEntry);
        for(int i=0; i < entries; i++) {
            printf("+- Entry %i\n", i+1);
            const char* relocType = "Unknown (Assembler Bug?)";
            if(relocation[i].type == BRANCH28) {
                relocType = "BRANCH28";
            } else if(relocation[i].type == PTR32) {
                relocType = "PTR32";
            } else if(relocation[i].type == LA32) {
                relocType = "LA32";
            }
            printf("|- Type: %s\n", relocType);
            printf("|- Source: <%s+0x%x>\n", segToString(relocation[i].srcSegment), relocation[i].srcOffset);
            printf("|- Destination: <%s+0x%x>\n", segToString(relocation[i].dstSegment), relocation[i].dstOffset);
        }
        free(image);
    } else if(strcmp(argv[1],"fwdump") == 0) {
        size_t imgSize;
        uint8_t* image = readImage(argv[4],&imgSize);
        if(!image) {
            fprintf(stderr, "Unable to read image!\n");
            free(image);
            return 1;
        }
        OkROHeader* header = (OkROHeader*)image;
        if(memcmp(image,"\x89OkamiRO",8) != 0) {
            fprintf(stderr, "Magic number is invalid\n");
            free(image);
            return 2;
        }
        uint32_t num = (uint32_t)strtol(argv[2],NULL,0);
        uint32_t num2 = (uint32_t)strtol(argv[3],NULL,0);
        preformRelocation(image,num,num2);
        writeImage(argv[5],image+sizeof(OkROHeader),imgSize-sizeof(OkROHeader)-getSize(header->reloc));
        free(image);
    } else if(strcmp(argv[1],"dump") == 0) {
        size_t imgSize;
        uint8_t* image = readImage(argv[3],&imgSize);
        if(!image) {
            fprintf(stderr, "Unable to read image!\n");
            free(image);
            return 1;
        }
        OkROHeader* header = (OkROHeader*)image;
        if(memcmp(image,"\x89OkamiRO",8) != 0) {
            fprintf(stderr, "Magic number is invalid\n");
            free(image);
            return 2;
        }
        uint32_t num = (uint32_t)strtol(argv[2],NULL,0);
        preformRelocation(image,num,0);
        writeImage(argv[4],image+sizeof(OkROHeader),imgSize-sizeof(OkROHeader)-getSize(header->reloc));
        free(image);
    }
    return 0;
}