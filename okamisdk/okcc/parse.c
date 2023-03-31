#include "cc.h"
#include "parse.h"
#include "lex.h"
#include <string.h>
#include <stdlib.h>

ASTNode_t* parseMakeSymbol(ASTNode_t** parent, char* name) {
    ASTNode_t* node = malloc((sizeof(ASTNode_t)-sizeof(void*))+strlen(name)+1);
    node->type = astSymbol;
    node->size = sizeof(ASTNode_t)-sizeof(void*)+strlen(name)+1;
    memcpy((char*)(&node->data),name,strlen(name)+1);
    (*parent) = realloc((*parent),(*parent)->size+sizeof(void*));
    *((ASTNode_t**)(((char*)(*parent))+((*parent)->size))) = node;
    (*parent)->size += sizeof(void*);
    return node;
}

ASTNode_t* parseMakeNumber(ASTNode_t** parent, unsigned int num) {
    ASTNode_t* node = malloc(sizeof(ASTNode_t)-sizeof(void*)+sizeof(unsigned int));
    node->type = astNumber;
    node->size = sizeof(ASTNode_t)-sizeof(void*)+sizeof(unsigned int);
    *((unsigned int*)&node->data) = num;
    (*parent) = realloc((*parent),(*parent)->size+sizeof(void*));
    *((ASTNode_t**)(((char*)(*parent))+((*parent)->size))) = node;
    (*parent)->size += sizeof(void*);
    return node;
}

ASTNode_t* parseMakeString(ASTNode_t** parent, char* name) {
    ASTNode_t* node = malloc(sizeof(ASTNode_t)-sizeof(void*)+strlen(name)+1);
    node->type = astString;
    node->size = sizeof(ASTNode_t)-sizeof(void*)+strlen(name)+1;
    memcpy((char*)(&node->data),name,strlen(name));
    (*parent) = realloc((*parent),(*parent)->size+sizeof(void*));
    *((ASTNode_t**)(((char*)(*parent))+((*parent)->size))) = node;
    (*parent)->size += sizeof(void*);
    return node;
}

ASTNode_t* parseMakeNode(ASTNode_t** parent) {
    ASTNode_t* node = malloc(sizeof(ASTNode_t)-sizeof(void*));
    node->type = astNode;
    node->size = sizeof(ASTNode_t)-sizeof(void*);
    if(parent != NULL) {
        (*parent) = realloc((*parent),(*parent)->size+sizeof(void*));
        *((ASTNode_t**)(((void*)(*parent))+((*parent)->size))) = node;
        (*parent)->size += sizeof(void*);
    }
    return node;
}

void parseDumpTree(ASTNode_t* initial) {
    int i, len;
    len = (initial->size-(sizeof(ASTNode_t)-sizeof(void*)))/sizeof(void*);
    fputs("\n(",stderr);
    for(i=0; i < len; i++) {
        switch(((ASTNode_t*)(&initial->data)[i])->type) {
            case astSymbol: {
                fputs((char*)&(((ASTNode_t*)(&initial->data)[i])->data),stderr);
                fputc(' ',stderr);
                break;
            }
            case astNumber: {
                fprintf(stderr,"%i ",*((unsigned int*)&(((ASTNode_t*)(&initial->data)[i])->data)));
                break;
            }
            case astString: {
                fprintf(stderr,"\"%s\" ", (char*)&(((ASTNode_t*)(&initial->data)[i])->data));
                break;
            }
            case astNode: {
                parseDumpTree(((ASTNode_t*)(&initial->data)[i]));
                break;
            }
            default: {
                fputs("??UNKNOWN NODE?? ",stderr);
                break;
            }
        }
    }
    fputs(")",stderr);
}

Parser_t* parseNew(char** srcBegin, int count) {
    Parser_t* parser = malloc(sizeof(void*)+(count*sizeof(void*)));
    parser->lexerCount = count;
    int i;
    for(i=0; i < count; i++) {
        unsigned long size;
        char* data = readFile(srcBegin[i],&size);
        if(data == NULL)
            ccErr(srcBegin[i],0,0,"Unable to Read!");
        parser->lexers[i] = lexNew(srcBegin[i],data);
    }
    parser->rootNode = parseMakeNode(NULL);
    parseMakeSymbol(&(parser->rootNode),"program");
    parseMakeSymbol(&(parser->rootNode),strtok(srcBegin[count],"."));
    while (strtok(NULL, ".")); /* Iterate through any other tokens
                                  that were generated. */
    parseDumpTree(parser->rootNode);
    return parser;
}

void parseRun(Parser_t* p) {
    int i;
    for(i=0;i < p->lexerCount;i++) {
        Token_t* tk = lexNext(p->lexers[i]);
        while(tk != NULL) {
            
            tk = lexNext(p->lexers[i]);
        }
    }
}