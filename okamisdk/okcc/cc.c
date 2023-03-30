#include <stdio.h>
#include "cc.h"
#include "notice.h"
#include "parse.h"
#include <stdlib.h>

int main(int argc,char *argv[]) {
    fputs(VERSION, stderr);
    fputs(CRIGHT, stderr);
    if(argc < 3) {
        fputs("usage: cc [file(s)] [asm_out]\n",stderr);
        return 1;
    }
    return 0;
}

void ccErr(char* file, unsigned long line, unsigned long col, char* err) {
    if(file == NULL) {
        fprintf(stderr,"\x1b[1;21mAnonymousBuffer(NO LOCATION) - %s\x1b[0m\n",err);
    } else if(line == 0) {
        fprintf(stderr,"\x1b[1;21m%s(NO LOCATION) - %s\x1b[0m\n",err);
    } else {
        fprintf(stderr,"\x1b[1;21m%s(%li:%li) - %s\x1b[0m\n",line,col,err);
    }
    exit(2);
}
char* readFile(char* path, unsigned long* size) {
    FILE* file = fopen(path,"rb");
    if(file == NULL)
        return NULL;
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    if(file != NULL) {
        *size = fsize;
    }
    fseek(file, 0, SEEK_SET);
    char* content = malloc(fsize+1);
    if(fread(content, fsize, 1, file) != 1) {
        free(content);
        return NULL;
    }
    content[fsize] = 0;
    fclose(file);
    return content;
}