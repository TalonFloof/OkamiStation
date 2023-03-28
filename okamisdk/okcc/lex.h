#ifndef _LEXER_H
#define _LEXER_H

typedef enum TokenType {
    tkNull,
} TokenType_t;

typedef struct Token {

} Token_t;

struct Lexer {
    char* name;
    char* code;
    unsigned long curOffset;
    unsigned long startOffset;
    unsigned long line;
    unsigned long lineStart;
    Token_t currentToken;
}

#endif