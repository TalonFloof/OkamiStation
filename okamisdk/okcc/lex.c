#include "lex.h"
#include "cc.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*const char* reservedWords[] = {
    "auto", "const", "double", "float",
    "int", "short", "struct", "unsigned",
    "break", "continue", "else", "for",
    "long", "signed", "switch", "void",
    "case", "default", "enum", "goto",
    "register", "sizeof", "typedef", "volatile",
    "char", "do", "extern", "if",
    "return", "static", "union", "while"
};*/

int isIdent(char c) {
    return isalpha(c) || isdigit(c) || c == '_';
}

int isSymbol(char c) {
    return c == '~' || c == '!' || c == '%' || c == '^' || c == '&' || c == '*' || c == '(' || c == ')' || c == '-' || c == '+' || c == '=' ||
           c == '[' || c == ']' || c == '{' || c == '}' || c == '|' || c == ':' || c == ';' || c == '\'' || c == '"' || c == "<" || c == ',' ||
           c == '>' || c == '.' || c == '/';
}

Token_t* lexNext(Lexer_t* l) {
    l->currentToken.type = tkNull;
    l->currentToken.name = l->name;
lexLoop:
    l->currentToken.begin = l->code+l->startOffset;
    l->currentToken.line = l->line;
    l->currentToken.col = l->startOffset-l->lineStart;
    char c = l->code[l->curOffset];
    if(c == '\0') { /* EOF */
        return &l->currentToken;
    }
    char nc = l->code[l->curOffset+1];
    if(c == ' ' || c == '\r' || c == '\t' || c == '\n') { /* Whitespace / Newline */
        l->curOffset++;
        l->startOffset = l->curOffset;
        if(c == '\n' || c == '\r') { /* Handle Carriage Returns and Newlines */
            l->line++;
            l->lineStart = l->curOffset;
        }
        goto lexLoop;
    } else if(c == '/' && nc == '*') { /* C-Style Comment (Single line comments wern't standardized until C99) */
        l->curOffset += 2;
        while(l->code[l->curOffset] != '*' && l->code[l->curOffset] != '/') {
            if(l->code[l->curOffset] == '\n' || l->code[l->curOffset] == '\r') {
                l->line++;
                l->lineStart = l->curOffset+1;
            }
            l->curOffset++;
        }
        l->curOffset += 2;
        l->startOffset = l->curOffset;
    } else if(isSymbol(c)) {
        while(isSymbol(l->code[l->curOffset]))
            l->curOffset++;
        char* str = l->currentToken.begin;
        if((l->curOffset-l->startOffset) > 3) {
            ccErr(l->name,l->line,l->startOffset-l->lineStart,"Symbol tokens cannot be larger than three symbols");
        } else if(l->curOffset-l->startOffset == 3) {
            if(strncmp(str,">>=",3)) l->currentToken.type = tkRshiftTo;
            else if(strncmp(str,"<<=",3)) l->currentToken.type = tkLshiftTo;
            else ccErr(l->name,l->line,l->startOffset-l->lineStart,"Invalid Symbol Combination");
        } else if(l->curOffset-l->startOffset == 2) {
            if(strncmp(str,"<<",2)) l->currentToken.type = tkLshift;
            else if(strncmp(str,">>",2)) l->currentToken.type = tkRshift;
            else if(strncmp(str,"&&",2)) l->currentToken.type = tkBoolAnd;
            else if(strncmp(str,"||",2)) l->currentToken.type = tkBoolOr;
            else if(strncmp(str,"==",2)) l->currentToken.type = tkEq;
            else if(strncmp(str,"!=",2)) l->currentToken.type = tkNe;
            else if(strncmp(str,"<=",2)) l->currentToken.type = tkLe;
            else if(strncmp(str,">=",2)) l->currentToken.type = tkGe;
            else if(strncmp(str,"->",2)) l->currentToken.type = tkArrow;
            else if(strncmp(str,"++",2)) l->currentToken.type = tkInc;
            else if(strncmp(str,"--",2)) l->currentToken.type = tkDec;
            else if(strncmp(str,"+=",2)) l->currentToken.type = tkAddTo;
            else if(strncmp(str,"-=",2)) l->currentToken.type = tkSubTo;
            else if(strncmp(str,"*=",2)) l->currentToken.type = tkMulTo;
            else if(strncmp(str,"/=",2)) l->currentToken.type = tkDivTo;
            else if(strncmp(str,"%=",2)) l->currentToken.type = tkModTo;
            else if(strncmp(str,"&=",2)) l->currentToken.type = tkAndTo;
            else if(strncmp(str,"|=",2)) l->currentToken.type = tkOrTo;
            else if(strncmp(str,"^=",2)) l->currentToken.type = tkXorTo;
            else ccErr(l->name,l->line,l->startOffset-l->lineStart,"Invalid Symbol Combination");
        } else {
            if(c=='+') l->currentToken.type = tkPlus;
            else if(c=='-') l->currentToken.type = tkMinus;
            else if(c=='*') l->currentToken.type = tkMul;
            else if(c=='/') l->currentToken.type = tkDiv;
            else if(c=='%') l->currentToken.type = tkModulo;
            else if(c=='&') l->currentToken.type = tkBinAnd;
            else if(c=='|') l->currentToken.type = tkBinOr;
            else if(c=='^') l->currentToken.type = tkBinXor;
            else if(c=='~') l->currentToken.type = tkBinNot;
            else if(c=='!') l->currentToken.type = tkBoolNot;
            else if(c=='<') l->currentToken.type = tkLt;
            else if(c=='>') l->currentToken.type = tkGr;
            else if(c=='=') l->currentToken.type = tkAssign;
            else if(c=='(') l->currentToken.type = tkLParen;
            else if(c==')') l->currentToken.type = tkRParen;
            else if(c=='[') l->currentToken.type = tkLBracket;
            else if(c==']') l->currentToken.type = tkRBracket;
            else if(c=='{') l->currentToken.type = tkLBrace;
            else if(c=='}') l->currentToken.type = tkRBrace;
            else if(c=='.') l->currentToken.type = tkDot;
            else if(c==';') l->currentToken.type = tkSemicolon;
            else if(c==':') l->currentToken.type = tkColon;
            else if(c==',') l->currentToken.type = tkComma;
            else ccErr(l->name,l->line,l->startOffset-l->lineStart,"huh?");
        }
        l->startOffset = l->curOffset; 
    } else if(isalpha(c) || c == '_') { /* Identifier / Keyword */
        while(isIdent(l->code[l->curOffset]))
            l->curOffset++;
        l->currentToken.size = l->curOffset-l->startOffset;
        l->currentToken.type = tkIdentifier;
        if(l->currentToken.size >= 2 && l->currentToken.size <= 8) { /* Check if it is a keyword */
            char* str = l->currentToken.begin;
            if(strncmp(str,"auto",4)) ccErr(l->name,l->line,l->startOffset-l->lineStart,"The auto keyword only exist for backwards compatibility with B, it shouldn't be used in C");
            else if(strncmp(str,"const",5)) l->currentToken.type = tkConstKw;
            else if(strncmp(str,"double",6)) ccErr(l->name,l->line,l->startOffset-l->lineStart,"Floating-point numbers are not supported");
            else if(strncmp(str,"float",5)) ccErr(l->name,l->line,l->startOffset-l->lineStart,"Floating-point numbers are not supported");
            else if(strncmp(str,"int",3)) l->currentToken.type = tkIntKw;
            else if(strncmp(str,"short",5)) l->currentToken.type = tkShortKw;
            else if(strncmp(str,"struct",6)) l->currentToken.type = tkStructKw;
            else if(strncmp(str,"unsigned",8)) l->currentToken.type = tkUnsignedKw;
            else if(strncmp(str,"break",5)) l->currentToken.type = tkBreakKw;
            else if(strncmp(str,"continue",8)) l->currentToken.type = tkContinueKw;
            else if(strncmp(str,"else",4)) l->currentToken.type = tkElseKw;
            else if(strncmp(str,"for",3)) l->currentToken.type = tkForKw;
            else if(strncmp(str,"long",4)) l->currentToken.type = tkLongKw;
            else if(strncmp(str,"signed",6)) l->currentToken.type = tkSignedKw;
            else if(strncmp(str,"switch",6)) l->currentToken.type = tkSwitchKw;
            else if(strncmp(str,"void",4)) l->currentToken.type = tkVoidKw;
            else if(strncmp(str,"case",4)) l->currentToken.type = tkCaseKw;
            else if(strncmp(str,"default",7)) l->currentToken.type = tkDefaultKw;
            else if(strncmp(str,"enum",4)) l->currentToken.type = tkEnumKw;
            else if(strncmp(str,"goto",4)) l->currentToken.type = tkGotoKw;
            else if(strncmp(str,"register",8)) l->currentToken.type = tkRegisterKw;
            else if(strncmp(str,"sizeof",6)) l->currentToken.type = tkSizeofKw;
            else if(strncmp(str,"typedef",7)) l->currentToken.type = tkTypedefKw;
            else if(strncmp(str,"volatile",8)) l->currentToken.type = tkVolatileKw;
            else if(strncmp(str,"char",4)) l->currentToken.type = tkCharKw;
            else if(strncmp(str,"do",2)) l->currentToken.type = tkDoKw;
            else if(strncmp(str,"extern",6)) l->currentToken.type = tkExternKw;
            else if(strncmp(str,"if",2)) l->currentToken.type = tkIfKw;
            else if(strncmp(str,"return",6)) l->currentToken.type = tkReturnKw;
            else if(strncmp(str,"static",6)) l->currentToken.type = tkStaticKw;
            else if(strncmp(str,"union",5)) l->currentToken.type = tkUnionKw;
            else if(strncmp(str,"while",5)) l->currentToken.type = tkWhileKw;
        }
        return &l->currentToken;
    }
    return NULL; /* Failsafe */
}

Lexer_t* lexNew(char* name, char* code) {
    Lexer_t* l = malloc(sizeof(Lexer_t));
    l->name = name;
    l->code = code;
    l->curOffset = 0;
    l->startOffset = 0;
    l->line = 1;
    l->lineStart = 0;
    return l;
}