/* Okami Relocatable Object Tool
 *
 * Created by TalonFox for the OkamiStation 1000.
 * Copyright (C) 2023 TalonFox, Licensed under the MIT License
 * This is a FOSS project.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
    unsigned int srcOffset;
    unsigned int dstOffset;
} RelocationEntry;

typedef struct {
    uint32_t nameoff;
    SegmentType segment;
    uint32_t offset;
    uint8_t external : 1;
    uint8_t libname : 1; /* For dynamic linking */
    uint8_t reserved : 6;
} Symbol;

const char* segToString(SegmentType type) {
    if(type == TEXT) {
        return ".text";
    } else if(type == RODATA) {
        return ".rodata";
    } else if(type == DATA) {
        return ".data";
    } else if(type == BSS) {
        return ".bss";
    } else if(type == EXTERN) {
        return ".symTable";
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
        if(relocation[i].dstSegment == EXTERN) {
            continue;
        }
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
            dstPtr = (uint32_t*)((uintptr_t)header+sizeof(OkROHeader)+relocation[i].srcOffset);
        } else if(relocation[i].srcSegment == RODATA) {
            dstPtr = (uint32_t*)((uintptr_t)header+sizeof(OkROHeader)+getSize(header->text)+relocation[i].srcOffset);
        } else if(relocation[i].srcSegment == DATA) {
            dstPtr = (uint32_t*)((uintptr_t)header+sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+relocation[i].srcOffset);
        } else if(relocation[i].srcSegment == BSS) {
            dstPtr = (uint32_t*)((uintptr_t)header+sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data)+relocation[i].srcOffset);
        }
        switch(relocation[i].type) {
            case BRANCH28: {
                /*uint32_t op = (*dstPtr) & 0xFC000000;
                *dstPtr = (((labelAddr >> 2) & 0x3FFFFFF) | op);*/
                fprintf(stderr, "The new BRANCH28 relocation is not supported yet\n");
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
        printf("info [object]: Get info about an Object.\n");
        printf("rinfo [object]: Get relocation info from an object.\n");
        printf("sinfo [object]: Get symbol info from an object.\n");
        printf("reloc [-noalign] [roBase] [rwBase] [object]: Relocates an object, this also strips unneeded relocation info.\n");
        printf("strip [object]: Removes unneeded symbol data from an object.\n");
        printf("setentry [entrySymbol] [object]: Sets the entrypoint to the given symbol.\n");
        printf("dump [object] [binary]: Convert an object into a raw binary.\n");
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
        printf("== %s ==\nVersion: %i\nText Size: %i bytes (~%i instructions)\nRoData Size: %i bytes\nData Size: %i bytes\nBSS Size: %i bytes\nRelocation Entries: %i\nSymbols: %i\nRead-Only Load Address: 0x%08x\nRead-Write Load Address: 0x%08x\nEntrypoint: <.text+0x%x>\n", argv[2], header->version, header->text, header->text/4, header->rodata, header->data, header->bss, header->reloc/sizeof(RelocationEntry), header->sym/sizeof(Symbol), header->roBase, header->rwBase, header->entry);
        free(image);
    } else if(strcmp(argv[1],"rinfo") == 0) {
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
        RelocationEntry* relocation = (RelocationEntry*)(((uintptr_t)header)+sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data));
        uint32_t entries = header->reloc/sizeof(RelocationEntry);
        for(int i=0; i < entries; i++) {
            printf("Entry #%i:\n", i+1);
            const char* relocType = "Unknown (Assembler Bug?)";
            if(relocation[i].type == BRANCH28) {
                relocType = "BRANCH28";
            } else if(relocation[i].type == PTR32) {
                relocType = "PTR32";
            } else if(relocation[i].type == LA32) {
                relocType = "LA32";
            }
            printf("  Type: %s\n", relocType);
            printf("  Source: <%s+0x%x>\n", segToString(relocation[i].srcSegment), relocation[i].srcOffset);
            printf("  Destination: <%s+0x%x>\n", segToString(relocation[i].dstSegment), relocation[i].dstOffset);
        }
        free(image);
    } else if(strcmp(argv[1],"sinfo") == 0) {
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
        Symbol* symbols = (Symbol*)(((uintptr_t)header)+sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data)+getSize(header->reloc));
        unsigned char* strs = (unsigned char*)(((uintptr_t)symbols)+getSize(header->sym));
        for(int i=0; i < header->sym/sizeof(Symbol); i++) {
            printf("Symbol #%i:\n", i+1);
            printf("  Name: %s\n", strs+symbols[i].nameoff);    
            printf("  Type: %s\n", symbols[i].libname?"Requested Library":(symbols[i].external?"External Symbol":"Global Symbol"));
            printf("  Location: <%s+0x%x>\n", segToString(symbols[i].segment), symbols[i].offset);
        }
        free(image);
    } else if(strcmp(argv[1],"reloc") == 0) {
        int noAlign = 0;
        if(strcmp(argv[2],"-noalign") == 0)
            noAlign = 1;
        size_t imgSize;
        uint8_t* image = readImage(argv[noAlign+4],&imgSize);
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
        uint32_t num1 = (uint32_t)strtoul(argv[noAlign+2],NULL,0);
        uint32_t num2 = (uint32_t)strtoul(argv[noAlign+3],NULL,0);
        if(num2 == 0) {
            num2 = num1+getSize(header->text)+getSize(header->rodata);
            if(!noAlign) {
                if((num2 & 0xfff) != 0)
                    num2 = ((num2>>12)+1)<<12;
            }
        }
        preformRelocation(image,num1,num2);
        FILE* finalImage = fopen(argv[noAlign+4],"wb");
        RelocationEntry* relocation = (RelocationEntry*)(((uintptr_t)header)+sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data));
        size_t oldRelocSize = getSize(header->reloc);
        header->reloc = 0;
        header->roBase = num1;
        header->rwBase = num2;
        for(int i=0; i < oldRelocSize/sizeof(RelocationEntry); i++) {
            if(relocation[i].dstSegment == EXTERN) {
                header->reloc += sizeof(RelocationEntry);
            }
        }
        fwrite(image,sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data),1,finalImage);
        for(int i=0; i < oldRelocSize/sizeof(RelocationEntry); i++) {
            if(relocation[i].dstSegment == EXTERN) {
                fwrite(relocation+i,sizeof(RelocationEntry),1,finalImage);
            }
        }
        size_t finalData = sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data)+oldRelocSize;
        fwrite(image+finalData,getSize(header->sym)+header->str,1,finalImage);
        fclose(finalImage);
        free(image);
    } else if(strcmp(argv[1],"strip") == 0) {
        size_t imgSize;
        uint8_t* image = readImage(argv[2],&imgSize);
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
        FILE* finalImage = fopen(argv[2],"wb");
        size_t oldSymSize = getSize(header->sym);
        unsigned char* newStringTable = NULL;
        header->str = 0;
        header->sym = 0;
        RelocationEntry* relocation = (RelocationEntry*)(((uintptr_t)header)+sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data));
        Symbol* symbols = (Symbol*)(((uintptr_t)relocation)+getSize(header->reloc));
        unsigned char* strs = (unsigned char*)(((uintptr_t)symbols)+oldSymSize);
        for(int i=0; i < oldSymSize/sizeof(Symbol); i++) {
            if(symbols[i].libname || symbols[i].external) {
                newStringTable = realloc(newStringTable,header->str+strlen(strs+symbols[i].nameoff)+1);
                memcpy(newStringTable+header->str,strs+symbols[i].nameoff,strlen(strs+symbols[i].nameoff)+1);
                symbols[i].nameoff = header->str;
                if(symbols[i].external) {
                    for(int j=0; j < header->reloc/sizeof(RelocationEntry); j++) {
                        if(relocation[j].dstSegment == EXTERN && relocation[j].dstOffset == i*sizeof(Symbol)) {
                            relocation[j].dstOffset = header->sym;
                        }
                    }
                }
                header->str += strlen(strs+symbols[i].nameoff)+1;
                header->sym += sizeof(Symbol);
            }
        }
        fwrite(image,sizeof(OkROHeader)+getSize(header->text)+getSize(header->rodata)+getSize(header->data)+getSize(header->reloc),1,finalImage);
        for(int i=0; i < oldSymSize/sizeof(Symbol); i++) {
            if(symbols[i].libname || symbols[i].external) {
                fwrite(symbols+i,sizeof(Symbol),1,finalImage);
            }
        }
        fwrite(newStringTable,header->str,1,finalImage);
        free(newStringTable);
        fclose(finalImage);
        free(image);
    } else if(strcmp(argv[1],"dump") == 0) {
        size_t imgSize;
        uint8_t* image = readImage(argv[2],&imgSize);
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
        writeImage(argv[3],image+sizeof(OkROHeader),getSize(header->text)+getSize(header->rodata)+getSize(header->data));
        free(image);
    }
    return 0;
}