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

uint8_t* readImage(const char* path) {
    FILE* file = fopen(path,"rb");
    if(file == NULL)
        return NULL;
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t* content = malloc(fsize);
    if (fread(content, fsize, 1, file) != 1)
        return NULL;
    fclose(file);
    return content;
}

int main(int argc, const char* argv[]) {
    if(argc == 1) {
        printf("Usage: okrotool [command]\n");
        printf("Commands:\n");
        printf("info [object]: Get Info about a Object.\n");
        printf("dump [-stripbss] [base] [object] [binary]: Convert an object into a raw binary.\n");
    } else if(strcmp(argv[1],"info") == 0) {
        uint8_t* image = readImage(argv[2]);
        if(!image) {
            fprintf(stderr, "Unable to read image!\n");
            return 1;
        }
        OkROHeader* header = (OkROHeader*)image;
        if(memcmp(image,"\x89OkamiRO",8) != 0) {
            fprintf(stderr, "Magic number is invalid\n");
            return 2;
        }
        printf("== %s ==\nVersion: %i\nText Size: %i bytes (~%i instructions)\nRoData Size: %i bytes\nData Size: %i bytes\nBSS Size: %i bytes\nRelocation Entries: %i\n", argv[2], header->version, header->text, header->text/4, header->rodata, header->data, header->bss, header->reloc/20);
    } else if(strcmp(argv[1],"dump") == 0) {
        int argvBase = 2;
        bool stripBSS = false;
        if(argv[2] == "-stripbss") {
            stripBSS = true;
            argvBase = 3;
        }
        
        uint8_t* image = readImage(argv[argvBase+1]);

    }
    return 0;
}