/*
Copyright (C) 2023 TalonFox

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

typedef enum {
    OKAY = 0xa072a572,
    ACTIVE = 0xbf0d34b6,
    DAMAGED = 0x5af5b64a,
} FennecFSState;

typedef struct {
    uint32_t icount; /* Inode Count */
    uint32_t journalsize; /* Journal Log Size (excludes metadata) */
    uint32_t ztagsize; /* Zone Tag Table Size (in Zones) */
    uint32_t zone; /* First block in zone */
    uint32_t zones; /* Number of zones */
    uint32_t zonesize; /* Zone Size (must be at least 1024) */
    uint32_t reserved;
    FennecFSState state; /* Filesystem State */
    uint64_t magic; /* "\x80Fennec\x80" */
    uint32_t revision; /* 1 */
} FennecSuperblock;

typedef struct {
    uint32_t mode; /* Inode Type and Permission Bits */
    uint32_t links; /* Number of Hard Links */
    uint32_t uid; /* User ID */
    uint32_t gid; /* Group ID */
    uint64_t size; /* Data Size */
    int64_t atime; /* Access Time */
    int64_t mtime; /* Modify Time */
    int64_t ctime; /* Status Change Time */
    uint32_t firstzone; /* First Allocated Zone of the File */
    uint8_t reserved[72]; /* Reserved for future metadata */
    uint32_t iconcolor; /* Color of the inode icon */
    uint8_t icon[128]; /* 32x32 pixel 1-bit icon bitmap */
} FennecInode;

typedef struct {
    uint32_t inodeid;
    char name[60];
} FennecDirEntry;

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

FennecInode* getInode(FennecSuperblock* super, uint32_t inode) {
    return (FennecInode*)((uintptr_t)super)+512+((inode-1)*sizeof(FennecInode));
}

uint32_t getFreeInode(FennecSuperblock* super) {
    return 0;
}

uint8_t* generateImage(uint64_t kib, uint64_t begin, uint64_t zonesize, uint64_t ratio) {
    uint64_t size = kib*1024;
    uint8_t* dat = malloc(size);
    uint32_t inodes = 0;
    if(ratio == 0) {
        if(kib >= 4194304) {
            inodes = size/65536;
        } else if(kib > 1048576) {
            inodes = size/32768;
        } else {
            inodes = size/16384;
        }
    } else {
        inodes = size/ratio;
    }
    uint32_t zones = (size-(begin-512)-(inodes*256))/zonesize;
    uint32_t ztt = ceil((double)(zones*4)/(double)zonesize);
    printf("mkfs.fennecfs %i Inodes, %i Zones (An extra %i Zones are reserved for the Zone Tag Table)\n", inodes, zones-ztt, ztt);
    FennecSuperblock* super = (FennecSuperblock*)(dat+begin);
    super->magic = 0x8063656E6E654680;
    super->revision = 1;
    super->state = OKAY;
    super->icount = inodes;
    super->journalsize = 0; /* To be implemented... */
    super->reserved = 0;
    super->zonesize = zonesize;
    super->zones = zones-ztt;
    super->zone = (begin/512)+1+(inodes/2)+(ztt*(zonesize/512));
    super->ztagsize = ztt;
    return dat;
}

int main(int argc, char** argv) {
    printf("FennecFSTool: The swiss-army knife of FennecFS\nCopyright (C) 2023 TalonFox, Licensed Under the MIT License. (see source code for more information)\n");
    if(argc < 3) {
        printf("Usage: fennecfstool [img] [command]\n");
        printf("Commands:\n");
        printf("newimage [KiB] [offset] [zonesize] [inoderatio]\n");
        printf("bootldr [bootimg]\n");
        printf("copy [src] [dst]\n");
        printf("mkdir [path]\n");
        printf("ls [path]\n");
        printf("read [path]\n");
        printf("delete [path]\n");
        printf("chmod [path] [mode]\n");
        printf("chown [path] [user] [group]\n");
        printf("cat [path]\n");
        printf("seticon [path] [icon]\n");
        printf("geticon [path]\n");
        return 1;
    } else {
        if(strcmp(argv[2],"newimage") == 0) {
            uint64_t kib = strtoull(argv[3],NULL,0);
            uint64_t inoderatio = 0;
            uint64_t zonesize = 1024;
            uint64_t startOffset = 0;
            if(argc >= 5) {
                startOffset = strtoull(argv[4],NULL,0)*512;
            }
            if(argc >= 6) {
                zonesize = strtoull(argv[5],NULL,0);
            }
            if(argc >= 7) {
                inoderatio = strtoull(argv[6],NULL,0);
            }
            uint8_t* data = generateImage(kib,startOffset,zonesize,inoderatio);
            //writeImage(argv[1],data,kib*1024);
            free(data);
        } else {
            if(strcmp(argv[2],"bootldr") == 0) {

            } else if(strcmp(argv[2],"copy") == 0) {
            }
        }
    }
    return 0;
}