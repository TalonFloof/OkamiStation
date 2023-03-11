/* Okami1041 Assembler
 *
 * Created by TalonFox for the OkamiStation 1000.
 * Copyright (C) 2023 TalonFox, Licensed under the MIT License
 * This is a FOSS project.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#if UINT_MAX < 4294967295U
#error "Target doesn't support 32-bit operations!"
#endif

typedef struct {
    unsigned long long magic;
    unsigned long long version;
    unsigned int text;
    unsigned int rodata;
    unsigned int data;
    unsigned int bss;
    unsigned int reloc;
    unsigned int sym;
    unsigned long long reserved;
} OkROHeader;

typedef enum {
    BRANCH28 = 0U,
    PTR32 = 1U,
    LA32 = 2U,
    REL16  /* This is a special relocation type that
              the assembler uses internally, it should
              never be found in the output. */
} RelocationType;

typedef enum {
    SEG_TEXT = 0U,
    SEG_RODATA = 1U,
    SEG_DATA = 2U,
    SEG_BSS = 3U,
    SEG_EXTERN = 4U, /* For an external symbol */
} SegmentType;

typedef struct {
    RelocationType type;
    SegmentType srcSegment;
    SegmentType dstSegment;
    unsigned int srcOffset;
    unsigned int dstOffset;
} RelocationEntry;

typedef struct {
  void* prev;
  SegmentType seg;
  unsigned int addr;
  char* name; /* Use &name */
} Label;

typedef enum {
  OP_NONE,
  OP_PSEUDO,
  OP_LOADIMM,
  OP_TWOREG,
  OP_REGREG,
  OP_FOURREG,
  OP_REGIMM,
  OP_LOADSTORE,
  OP_BRANCH28,
  OP_KMCALL
} OpType;

struct OkamiOpcode {
  int opcode;
  const char* name;
  OpType arg1;
};

const char* OkamiRegisters[] = {"zero","a0","a1","a2","a3","a4","a5","a6","a7","t0","t1","t2","t3","t4","t5","t6","t7","s0","s1","s2","s3","s4","s5","s6","s7","s8","s9","gp","kr","fp","sp","ra"};

const struct OkamiOpcode OkamiInstructions[] = {
  /* SECTION 1 */
  { 0,"add",OP_REGREG},
  { 1,"sub",OP_REGREG},
  { 2,"and",OP_REGREG},
  { 3,"or",OP_REGREG},
  { 4,"xor",OP_REGREG},
  { 5,"sll",OP_REGREG},
  { 6,"srl",OP_REGREG},
  { 7,"sra",OP_REGREG},
  { 8,"slt",OP_REGREG},
  { 9,"sltu",OP_REGREG},
  {10,"mul",OP_FOURREG},
  {11,"mulu",OP_FOURREG},
  {12,"div",OP_FOURREG},
  {13,"divu",OP_FOURREG},
  /* SECTION 2 */
  {16,"addi",OP_REGIMM},
  {17,"andi",OP_REGIMM},
  {18,"ori",OP_REGIMM},
  {19,"xori",OP_REGIMM},
  {20,"slli",OP_REGIMM},
  {21,"srli",OP_REGIMM},
  {22,"srai",OP_REGIMM},
  {23,"slti",OP_REGIMM},
  {24,"sltiu",OP_REGIMM},
  {25,"lui",OP_LOADIMM},
  /* SECTION 3 */
  {32,"b",OP_BRANCH28},
  {33,"bl",OP_BRANCH28},
  {34,"blr",OP_TWOREG},
  {35,"beq",OP_REGIMM},
  {36,"bne",OP_REGIMM},
  {37,"bge",OP_REGIMM},
  {38,"blt",OP_REGIMM},
  {39,"bgeu",OP_REGIMM},
  {40,"bltu",OP_REGIMM},
  /* SECTION 4 */
  {48,"lb",OP_LOADSTORE},
  {49,"lbu",OP_LOADSTORE},
  {50,"lh",OP_LOADSTORE},
  {51,"lhu",OP_LOADSTORE},
  {52,"lw",OP_LOADSTORE},
  {53,"sb",OP_LOADSTORE},
  {54,"sh",OP_LOADSTORE},
  {55,"sw",OP_LOADSTORE},
  /* EXTENDED REGISTERS */
  {60,"mfex",OP_LOADIMM},
  {61,"mtex",OP_LOADIMM},
  /* TRAPS */
  {62,"kcall",OP_KMCALL},
  {62,"mcall",OP_KMCALL},
  {63,"rft",OP_NONE},
  /* PSEUDO-INSTRUCTIONS */
  {0,"nop",OP_NONE},
  {-2,"li",OP_PSEUDO},
  {-3,"la",OP_PSEUDO},
  {-4,"br",OP_PSEUDO},
  {-5,"mv",OP_PSEUDO}
};

/*****VARIABLES*****/
unsigned int* text = NULL;
unsigned int textSize = 0;
unsigned int textCapacity = 0;
unsigned char* rodata = NULL;
unsigned int rodataSize = 0;
unsigned char* data = NULL;
unsigned int dataSize = 0;
unsigned int bss = 0;

SegmentType curSegment = SEG_TEXT;
/*******************/

unsigned int addToSegment(void* buf, unsigned int len) {
  switch(curSegment) {
    case SEG_TEXT: {
      if(textSize+len>textCapacity) {
        textCapacity+=1024;
        text = (unsigned int*)realloc(text,4*textCapacity);
      }
      memcpy(&text[textSize],buf,len);
      textSize+=(len/4);
      return textSize-(len/4);
    }
    case SEG_RODATA: {
      rodata = realloc(rodata,rodataSize+len);
      memcpy(&rodata[rodataSize],buf,len);
      rodataSize+=len;
      return rodataSize-len;
    }
    case SEG_DATA: {
      data = realloc(data,dataSize+len);
      memcpy(&data[dataSize],buf,len);
      dataSize+=len;
      return dataSize-len;
    }
    case SEG_BSS: {
      unsigned int ret = bss;
      bss += len;
      return ret;
    }
  }
}

unsigned int getSegmentSize() {
  switch(curSegment) {
    case SEG_TEXT:
      return textSize*4;
    case SEG_RODATA:
      return rodataSize;
    case SEG_DATA:
      return dataSize;
    case SEG_BSS:
      return bss;
    default:
      return 0;
  }
}

unsigned char* readFile(const char* path, unsigned long* fileSize) {
    FILE* file = fopen(path,"rb");
    if(file == NULL)
        return NULL;
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    if(fileSize != NULL) {
        *fileSize = fsize;
    }
    fseek(file, 0, SEEK_SET);
    char* content = malloc(fsize);
    if(fread(content, fsize, 1, file) != 1)
        return NULL;
    fclose(file);
    return content;
}

void Error(char* name, long line, long col, char *str) {
  fprintf(stderr, "\x1b[1;31m%s %li:%li - %s\x1b[0m\n", name, line, col, str);
  exit(1);
}

int iswhitespace(char val) {
  return val == ' ' || val == '\t' || val == '\r' || val == '\n';
}

void Assemble(char* name, char* data) {
  unsigned long len = strlen(data);
  unsigned long i = 0;
  unsigned long line = 1;
  unsigned long lineStart = 0;
  while(i<len) {
    if(data[i] == '\n') {
      i++;
      line++;
      lineStart = i;
    } else if(iswhitespace(data[i])) {
      i++;
    } else if(data[i] == '.') { /*Internal Symbol*/
      unsigned long start = i;
      while(!iswhitespace(data[i])) {i++;}
      unsigned long length = i-start;
      if(data[i-1] == ':') { /*Oh wait, its actually a local label*/

      } else if(strncmp(data+start,".global",length) == 0) {
      } else if(strncmp(data+start,".extern",length) == 0) {
      } else if(strncmp(data+start,".align",length) == 0) {
      } else if(strncmp(data+start,".byte",length) == 0) {
      } else if(strncmp(data+start,".short",length) == 0) {
      } else if(strncmp(data+start,".word",length) == 0) {
      } else if(strncmp(data+start,".fill",length) == 0) {
      } else if(strncmp(data+start,".string",length) == 0) {
      } else if(strncmp(data+start,".text",length) == 0) {
      } else if(strncmp(data+start,".rodata",length) == 0) {
      } else if(strncmp(data+start,".data",length) == 0) {
      } else if(strncmp(data+start,".bss",length) == 0) {
      } else {
        Error(name,line,start-lineStart,"Unknown Assembler Keyword");
      }
    } else if(isalnum(data[i]) || data[i] == '_') {
      unsigned long start = i;
      while(isalnum(data[i]) || data[i] == '_') {i++;}
      unsigned long length = i-start;
      if(data[i] == ':') {
        i++;
      } else {
        int index;
        for(index = 0; index < sizeof(OkamiInstructions)/sizeof(struct OkamiOpcode); index++) {
          if(strncmp(data+start,OkamiInstructions[index].name,length) == 0) {
            i+=2;
            int j;
            ArgType* argPtr = (ArgType*)(&(OkamiInstructions[index].arg1));
            for(j=0; j < 4; j++) {
              switch(argPtr[j]) {
                case ARG_IMM: {
                  unsigned long start = i;
                  int isNumber = 1;
                  while(isalnum(data[i])) {
                    if((i <= '0' || i >= '9') && (i <= 'a' || i >= 'f') && (i <= 'A' || i >= 'F') && i != "x" && i != "o") {
                      isNumber = 0;
                    }
                    i++;
                  }
                  unsigned long length = i-start;
                  if(isNumber) {
                    char* buf = malloc(length+1);
                    memcpy(buf,data+start,length);
                    unsigned int val = (unsigned int)((int)strtol(buf,NULL,0));
                    free(buf);
                  } else {
                    /* Add Label Relocation */
                    
                  }
                  break;
                }
                case ARG_REG: {
                  break;
                }
                case ARG_LOADSTORE: {
                  break;
                }
              }
            }
            goto next;
          }
        }
        Error(name,line,i-lineStart,"Unknown Okami Instruction");
      }
    } else if(data[i] == '/' && data[i+1] == '*') {
      i+=2;
      while(data[i] != '/' && data[i-1] != '*') {i++;}
      i++;
    } else {
      Error(name,line,i-lineStart,"Unknown Token");
    }
next:
    while(0);
  }
  free(data);
}

int main(int argc, char **argv) {
  if(argc < 3) {
    printf("Usage: okas [infile] [outfile]\n");
    return 0;
  }
  Assemble(argv[1],readFile(argv[1],NULL));
  free(text);
  free(rodata);
  free(data);
  return 0;
}