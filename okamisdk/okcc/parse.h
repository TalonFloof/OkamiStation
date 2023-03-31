#ifndef _PARSER_H
#define _PARSER_H
#include "lex.h"

typedef enum {
    astNull,
    astSymbol,
    astNumber,
    astString,
    astNode,
} ASTSymType;

typedef struct ASTNode {
    ASTSymType type;
    unsigned long size;
    void* data; /* This doesn't actually point to anything,
                   rather it marks the beginning of the data */
} ASTNode_t;

typedef struct Parser {
    ASTNode_t* rootNode;
    int lexerCount;
    Lexer_t* lexers[];
} Parser_t;

Parser_t* parseNew(char* srcBegin[], int count);

#endif