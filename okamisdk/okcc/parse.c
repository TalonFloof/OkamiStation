#include "parse.h"
#include "lex.h"
#include <stdlib.h>

Parser_t* parseNew(char* srcBegin[], int count) {
    Parser_t* parser = malloc(sizeof(void*)+(count*sizeof(void*)));
    int i;
    for(i=0; i < count; i++) {
        unsigned long size;
        char* data = readFile(size);
        parser->lexers[i] = lexNew(srcBegin[i],NULL);
    }
    return parser;
}