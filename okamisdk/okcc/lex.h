#ifndef _LEXER_H
#define _LEXER_H

typedef enum TokenType {
    tkNull,
    tkInvalid,
    tkIdentifier,
    tkString,
    tkNumber,
    tkCharacter,

    tkConstKw,
    tkIntKw,
    tkShortKw,
    tkStructKw,
    tkUnsignedKw,
    tkBreakKw,
    tkContinueKw,
    tkElseKw,
    tkForKw,
    tkLongKw,
    tkSignedKw,
    tkSwitchKw,
    tkVoidKw,
    tkCaseKw,
    tkDefaultKw,
    tkEnumKw,
    tkGotoKw,
    tkRegisterKw,
    tkSizeofKw,
    tkTypedefKw,
    tkVolatileKw,
    tkCharKw,
    tkDoKw,
    tkExternKw,
    tkIfKw,
    tkReturnKw,
    tkStaticKw,
    tkUnionKw,
    tkWhileKw,

    tkPlus,
    tkMinus,
    tkMul,
    tkDiv,
    tkModulo,
    tkBinAnd,
    tkBinOr,
    tkBinXor,
    tkBinNot,
    tkLshift,
    tkRshift,
    tkBoolAnd,
    tkBoolOr,
    tkBoolNot,
    tkEq,
    tkNe,
    tkLt,
    tkGr,
    tkLe,
    tkGe,

    tkAssign,
    tkInc,
    tkDec,
    tkAddTo,
    tkSubTo,
    tkMulTo,
    tkDivTo,
    tkModTo,
    tkAndTo,
    tkOrTo,
    tkXorTo,
    tkLshiftTo,
    tkRshiftTo,

    tkLParen,
    tkRParen,
    tkLBracket,
    tkRBracket,
    tkLBrace,
    tkRBrace,

    tkDot,
    tkArrow,
    tkSemicolon,
    tkColon,
    tkComma
} TokenType_t;

typedef struct Token {
    TokenType_t type;
    char* name;
    char* begin;
    unsigned long size;
    unsigned long line;
    unsigned long col;
} Token_t;

typedef struct Lexer {
    char* name;
    char* code;
    unsigned long curOffset;
    unsigned long startOffset;
    unsigned long line;
    unsigned long lineStart;
    Token_t currentToken;
} Lexer_t;

Token_t* lexNext(Lexer_t* l);
Lexer_t* lexNew(char* name, char* code);

#endif