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
#include <sys/time.h>

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
    unsigned int str;
    unsigned int entry;
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
  void* next;
  RelocationType type;
  SegmentType srcSegment;
  unsigned int offset;
  SegmentType dstSegment;
  unsigned int dstOffset;
  char* labelName;
} IntRelocEntry;

typedef struct {
  void* prev;
  void* next;
  SegmentType seg;
  unsigned int offset;
  char* name;
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
  OpType type;
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
  { 6,"sra",OP_REGREG},
  { 7,"slt",OP_REGREG},
  { 8,"sltu",OP_REGREG},
  { 9,"mul",OP_FOURREG},
  { 9,"mulu",OP_FOURREG},
  {10,"div",OP_FOURREG},
  {10,"divu",OP_FOURREG},
  /* SECTION 2 */
  {16,"addi",OP_REGIMM},
  {17,"andi",OP_REGIMM},
  {18,"ori",OP_REGIMM},
  {19,"xori",OP_REGIMM},
  {20,"slli",OP_REGIMM},
  {21,"srli",OP_REGIMM},
  {21,"srai",OP_REGIMM},
  {22,"slti",OP_REGIMM},
  {23,"sltiu",OP_REGIMM},
  {24,"lui",OP_LOADIMM},
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
  { 0,"nop",OP_NONE},
  {18,"li",OP_LOADIMM},
  {-1,"la",OP_PSEUDO},
  {-2,"br",OP_PSEUDO},
  { 0,"mv",OP_TWOREG}
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
Label* labelHead = NULL;
Label* labelTail = NULL;
IntRelocEntry* relocHead = NULL;
IntRelocEntry* relocTail = NULL;
unsigned int relocSize = 0;

char* curLabel = NULL;
SegmentType curSegment = SEG_TEXT;
/*******************/

unsigned int addToSegment(void* buf, unsigned int len) {
  switch(curSegment) {
    case SEG_TEXT: {
      if(textSize+(len/4)>textCapacity) {
        textCapacity+=1024;
        text = (unsigned int*)realloc(text,4*textCapacity);
      }
      memcpy(&text[textSize],buf,len);
      textSize+=(len/4);
      return (textSize-(len/4))*4;
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

void addReloc(RelocationType type, char* labelName) {
  char* label = labelName;
  if(labelName[0] == '.') {
    label = malloc(strlen(labelName)+strlen(curLabel)+1);
    memset(label,0,strlen(labelName)+strlen(curLabel)+1);
    memcpy(label,curLabel,strlen(curLabel));
    memcpy(label+strlen(curLabel),labelName,strlen(labelName));
    free(labelName);
  }
  IntRelocEntry* r = malloc(sizeof(IntRelocEntry));
  if(relocHead == NULL) {
    relocHead = r;
    relocTail = r;
  } else if(relocTail != NULL) {
    relocTail->next = r;
    r->prev = relocTail;
    relocTail = r;
  }
  r->labelName = label;
  r->srcSegment = curSegment;
  r->offset = getSegmentSize();
  r->type = type;
  relocSize++;
}

void addLabel(char* labelName) {
  Label* l = malloc(sizeof(Label));
  if(labelHead == NULL) {
    labelHead = l;
    labelTail = l;
  } else if(labelTail != NULL) {
    labelTail->next = l;
    l->prev = labelTail;
    labelTail = l;
  }
  l->seg = curSegment;
  l->offset = getSegmentSize();
  l->name = labelName;
}

unsigned int parseImm(OpType type, char* data, unsigned long* i) {
  unsigned long start = *i;
  int isNumber = 1;
  while(isalnum(data[*i]) || data[*i] == '_' || data[*i] == '.' || data[*i] == '-') {
    if(isNumber) {
      if((data[*i] < '0' || data[*i] > '9') && (data[*i] < 'a' || data[*i] > 'f') && (data[*i] < 'A' || data[*i] > 'F') && data[*i] != 'x' && data[*i] != 'o' && data[*i] != '-') {
        isNumber = 0;
      }
    }
    (*i)++;
  }
  unsigned long length = (*i)-start;
  if(data[*i] == ',') {
    if(data[(*i)+1] == ' ') {
      (*i)+=2;
    } else {
      (*i)++;
    }
  }
  char* buf = malloc(length+1);
  memset(buf,0,length+1);
  memcpy(buf,data+start,length);
  if(isNumber) {
    unsigned int val = (unsigned int)((int)strtol(buf,NULL,0));
    free(buf);
    return val;
  } else {
    /* Add Label Relocation */
    if(type == OP_BRANCH28) {
      addReloc(BRANCH28,buf);
    } else if(type == OP_LOADIMM) {
      addReloc(LA32,buf);
    } else if(type == OP_REGIMM) {
      addReloc(REL16,buf);
    }
    return 0;
  }
}

char* parseString(char* data, unsigned long* i) {
  if(data[*i] != '"') {
    return NULL;
  } else {
    (*i)++;
  }
  unsigned int start = *i;
  while(data[*i] != '"') {(*i)++;}
  unsigned length = (*i)-start;
  (*i)++;
  char* buf = malloc(length);
  memset(buf,0,length);
  memcpy(buf,data+start,length);
  return buf;
}

int parseReg(char* data, unsigned long* i) {
  unsigned long start = *i;
  while(isalnum(data[*i])) {(*i)++;}
  unsigned long length = (*i)-start;
  int reg;
  for(reg=0; reg < 32; reg++) {
    if(strncmp(data+start,OkamiRegisters[reg],length) == 0) {
      if(data[*i] == ',') {
        if(data[(*i)+1] == ' ') {
          (*i)+=2;
        } else {
          (*i)++;
        }
      }
      return reg;
    }
  }
  if(data[*i] == ',') {
    if(data[(*i)+1] == ' ') {
      (*i)+=2;
    } else {
      (*i)++;
    }
  }
  return -1;
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
        int lbLen = strlen(curLabel);
        char* buf = malloc(length+lbLen);
        memset(buf,0,length+lbLen);
        memcpy(buf,curLabel,lbLen);
        memcpy(buf+lbLen,data+start,length-1);
        addLabel(buf);
      } else if(strncmp(data+start,".global",length) == 0) {
      } else if(strncmp(data+start,".extern",length) == 0) {
        Error(name,line,start-lineStart,".extern is not implemented yet!");
      } else if(strncmp(data+start,".align",length) == 0) {
        i++;
        unsigned int num = parseImm(OP_NONE,data,&i);
        if(getSegmentSize() % num > 0) {
          unsigned int zero = 0;
          addToSegment(&zero,num-(getSegmentSize()%num));
        }
      } else if(strncmp(data+start,".resb",length) == 0) {
        i++;
        unsigned int val = (unsigned int)parseImm(OP_NONE,data,&i);
        addToSegment(NULL,val);
      } else if(strncmp(data+start,".byte",length) == 0) {
        i++;
        unsigned char val = (unsigned char)parseImm(OP_NONE,data,&i);
        addToSegment(&val,1);
      } else if(strncmp(data+start,".short",length) == 0) {
        i++;
        unsigned short val = (unsigned short)parseImm(OP_NONE,data,&i);
        addToSegment(&val,2);
      } else if(strncmp(data+start,".word",length) == 0) {
        i++;
        unsigned int val = parseImm(OP_NONE,data,&i);
        addToSegment(&val,4);
      } else if(strncmp(data+start,".fill",length) == 0) {
        i++;
        unsigned int size = parseImm(OP_NONE,data,&i);
        unsigned int dat = parseImm(OP_NONE,data,&i);
        unsigned char* buf = malloc(size);
        memset(buf,dat,size);
        addToSegment(&buf,size);
        free(buf);
      } else if(strncmp(data+start,".string",length) == 0) {
        i++;
        char* str = parseString(data,&i);
        addToSegment(str,strlen(str)+1);
        free(str);
      } else if(strncmp(data+start,".ascii",length) == 0) {
        i++;
        char* str = parseString(data,&i);
        addToSegment(str,strlen(str));
        free(str);
      } else if(strncmp(data+start,".include_bin",length) == 0) {
        i++;
        char* name = parseString(data,&i);
        unsigned long size = 0;
        unsigned char* file = readFile(name,&size);
        if(file != NULL) {
          addToSegment(file,size);
          free(file);
        } else {
          Error(name,0,0,"Failed to read file!");
        }
        free(name);
      } else if(strncmp(data+start,".text",length) == 0) {
        curSegment = SEG_TEXT;
      } else if(strncmp(data+start,".rodata",length) == 0) {
        curSegment = SEG_RODATA;
      } else if(strncmp(data+start,".data",length) == 0) {
        curSegment = SEG_DATA;
      } else if(strncmp(data+start,".bss",length) == 0) {
        curSegment = SEG_BSS;
      } else {
        Error(name,line,start-lineStart,"Unknown Assembler Keyword");
      }
    } else if(isalnum(data[i]) || data[i] == '_') {
      unsigned long start = i;
      while(isalnum(data[i]) || data[i] == '_') {i++;}
      unsigned long length = i-start;
      if(data[i] == ':') {
        i++;
        char* buf = malloc(length+1);
        memset(buf,0,length+1);
        memcpy(buf,data+start,length);
        curLabel = buf;
        addLabel(buf);
      } else {
        int index;
        for(index = 0; index < sizeof(OkamiInstructions)/sizeof(struct OkamiOpcode); index++) {
          if(strncmp(data+start,OkamiInstructions[index].name,length) == 0) {
            i++;
            switch(OkamiInstructions[index].type) {
              case OP_NONE: {
                unsigned int opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26);
                addToSegment(&opcode,4);
                break;
              }
              case OP_REGREG: {
                int reg1 = parseReg(data,&i);
                int reg2 = parseReg(data,&i);
                int reg3 = parseReg(data,&i);
                if(reg1 == -1 || reg2 == -1 || reg3 == -1) {Error(name,line,i-lineStart,"Invalid Register");}
                unsigned int opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26) 
                                      | (((unsigned int)reg3) << 21)
                                      | (((unsigned int)reg2) << 16)
                                      | (((unsigned int)reg1) << 11)
                                      | (index == 7 ? 0x400 : 0);
                addToSegment(&opcode,4);
                break;
              }
              case OP_REGIMM: {
                int reg1 = parseReg(data,&i);
                int reg2 = parseReg(data,&i);
                unsigned int imm = parseImm(OkamiInstructions[index].type,data,&i);
                if(reg1 == -1 || reg2 == -1) {Error(name,line,i-lineStart,"Invalid Register");}
                unsigned int opcode = 0;
                if(OkamiInstructions[index].opcode == 16) {
                  opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26) 
                           | (((unsigned int)reg2) << 21)
                           | (((unsigned int)reg1) << 16)
                           | (unsigned int)((unsigned short)((short)((int)imm)));
                } else {
                  opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26) 
                           | (((unsigned int)reg2) << 21)
                           | (((unsigned int)reg1) << 16)
                           | ((index == 20) ? 0x8000 : 0)
                           | imm;
                }
                addToSegment(&opcode,4);
                break;
              }
              case OP_TWOREG: {
                int reg1 = parseReg(data,&i);
                int reg2 = parseReg(data,&i);
                if(reg1 == -1 || reg2 == -1) {Error(name,line,i-lineStart,"Invalid Register");}
                unsigned int opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26)
                                      | (((unsigned int)reg1) << 16)
                                      | (((unsigned int)reg2) << 11);
                addToSegment(&opcode,4);
                break;
              }
              case OP_FOURREG: {
                int reg1 = parseReg(data,&i);
                int reg2 = parseReg(data,&i);
                int reg3 = parseReg(data,&i);
                int reg4 = parseReg(data,&i);
                if(reg1 == -1 || reg2 == -1 || reg3 == -1 || reg4 == -1) {Error(name,line,i-lineStart,"Invalid Register");}
                unsigned int opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26) 
                                      | (((unsigned int)reg4) << 21)
                                      | (((unsigned int)reg3) << 16)
                                      | (((unsigned int)reg2) << 11)
                                      | (((unsigned int)reg1) << 6)
                                      | (((index % 2) == 1) ? 0x8 : 0);
                addToSegment(&opcode,4);
                break;
              }
              case OP_LOADIMM: {
                int reg1 = parseReg(data,&i);
                unsigned int imm = parseImm(OkamiInstructions[index].type,data,&i);
                if(reg1 == -1) {Error(name,line,i-lineStart,"Invalid Register");}
                unsigned int opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26)
                                      | (((unsigned int)reg1) << (OkamiInstructions[index].opcode == 60 ? 21 : 16))
                                      | (imm&0xFFFF);
                addToSegment(&opcode,4);
                break;
              }
              case OP_BRANCH28: {
                unsigned int imm = parseImm(OkamiInstructions[index].type,data,&i);
                unsigned int opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26)
                                      | (imm>>2);
                addToSegment(&opcode,4);
                break;
              }
              case OP_KMCALL: {
                unsigned int imm = parseImm(OkamiInstructions[index].type,data,&i);
                unsigned int opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26)
                                      | (index == 44 ? 0x2000000 : 0)
                                      | imm;
                addToSegment(&opcode,4);
                break;
              }
              case OP_LOADSTORE: {
                unsigned int reg1 = parseReg(data,&i);
                short imm = (short)parseImm(OkamiInstructions[index].type,data,&i);
                if(data[i] != '(') {
                  Error(name,line,i-lineStart,"Invalid Load/Store instruction syntax");
                } else {i++;}
                unsigned int reg2 = parseReg(data,&i);
                if(data[i] != ')') {
                  Error(name,line,i-lineStart,"Invalid Load/Store instruction syntax");
                } else {i++;}
                if(reg1 == -1 || reg2 == -1) {Error(name,line,i-lineStart,"Invalid Register");}
                unsigned int opcode = (((unsigned int)OkamiInstructions[index].opcode) << 26)
                                      | (((unsigned int)reg1) << 21)
                                      | (((unsigned int)reg2) << 16)
                                      | (unsigned int)((unsigned short)imm);
                addToSegment(&opcode,4);
                break;
              }
              case OP_PSEUDO: {
                switch(OkamiInstructions[index].opcode) {
                  case -1: { /*la*/
                    int reg1 = parseReg(data,&i);
                    if(reg1 == -1) {Error(name,line,i-lineStart,"Invalid Register");}
                    unsigned int imm = parseImm(OP_LOADIMM,data,&i);
                    unsigned int opcode = 0x60000000 | (((unsigned int)reg1) << 16) | (imm >> 16);
                    addToSegment(&opcode,4);
                    opcode = 0x48000000 | (((unsigned int)reg1) << 16) | (((unsigned int)reg1) << 21) | (imm & 0xFFFF);
                    addToSegment(&opcode,4);
                    break;
                  }
                  case -2: { /*br*/
                    int reg1 = parseReg(data,&i);
                    if(reg1 == -1) {Error(name,line,i-lineStart,"Invalid Register");}
                    unsigned int opcode = 0x88000000
                                          | (((unsigned int)reg1) << 11);
                    addToSegment(&opcode,4);
                    break;                     
                  }
                }
                break;
              }
              default: {
                Error(name,line,i-lineStart,"Unknown Okami Instruction Type");
                break;
              }
            }
            goto next;
          }
        }

        Error(name,line,i-lineStart,"Unknown Okami Instruction");
      }
    } else if(data[i] == '/' && data[i+1] == '*') {
      i+=2;
      while(!(data[i] == '/' && data[i-1] == '*')) {
        if(data[i] == '\n') {
          line++;
          lineStart = i+1;
        }
        i++;
      }
      i++;
    } else if(data[i] != '\0') {
      Error(name,line,i-lineStart,"Unknown Token");
    }
next:
    while(0);
  }
  free(data);
}

unsigned int GenObject(char* out) {
  IntRelocEntry* index = relocHead;
  while(index != NULL) {
    void* next = index->next;
    if(index->type == REL16) {
      relocSize--;
      Label* lind = labelHead;
      while(lind != NULL) {
        if(strcmp(lind->name,index->labelName) == 0) {
          int jump = ((lind->offset)-((index->offset)+4))/4;
          *((short*)(&text[index->offset/4])) = ((short)jump);
          break;
        }
        lind = lind->next;
      }
      if(lind == NULL) {
        Error(index->labelName,0,0,"Label has no matching definition");
      }
    }
    index = next;
  }
  FILE* outfile = fopen(out,"wb");
  if(outfile == NULL) {
    Error(out,0,0,"Unable to open!");
  }
  OkROHeader header;
  memcpy((unsigned char*)&header,"\x89OkamiRO\x01\0\0\0\0\0\0\0",16);
  header.text = textSize*4;
  header.rodata = rodataSize;
  header.data = dataSize;
  header.bss = bss;
  header.reloc = relocSize*sizeof(RelocationEntry);
  header.sym = 0;
  header.str = 0;
  header.entry = 0;
  fwrite(&header,sizeof(OkROHeader),1,outfile);
  unsigned int zero = 0;
  fwrite(text,textSize*4,1,outfile);
  fwrite(rodata,rodataSize,1,outfile);
  if(rodataSize % 4 != 0)
    fwrite(&zero,4-(rodataSize%4),1,outfile);
  fwrite(data,dataSize,1,outfile);
  if(dataSize % 4 != 0)
    fwrite(&zero,4-(dataSize%4),1,outfile);
  while(relocHead != NULL) {
    if(relocHead->type != REL16) {
      Label* lind = labelHead;
      while(lind != NULL) {
        if(strcmp(lind->name,relocHead->labelName) == 0) {
          RelocationEntry entry;
          entry.type = relocHead->type;
          entry.srcSegment = relocHead->srcSegment;
          entry.srcOffset = relocHead->offset;
          entry.dstSegment = lind->seg;
          entry.dstOffset = lind->offset;
          fwrite(&entry,sizeof(RelocationEntry),1,outfile);
          break;
        }
        lind = lind->next;
      }
      if(lind == NULL) {
        Error(index->labelName,0,0,"Label has no matching definition");
      }
    }
    void* next = relocHead->next;
    /*free(relocHead->labelName);
    free(relocHead);*/
    relocHead = next;
  }
  long size = ftell(outfile);
  fclose(outfile);
  /*while(labelHead != NULL) {
    void* next = labelHead->next;
    free(labelHead->name);
    free(labelHead);
    labelHead = next;
  }*/
  return size;
}

int main(int argc, char **argv) {
  struct timeval startTime, endTime;
  if(argc < 3) {
    printf("Usage: okas [infile] [outfile]\n");
    return 0;
  }
  gettimeofday(&startTime, 0);
  Assemble(argv[1],readFile(argv[1],NULL));
  long fileSize = GenObject(argv[2]);
  gettimeofday(&endTime, 0);
  free(text);
  free(rodata);
  free(data);
  double startTS = ((double)startTime.tv_sec)+((double)startTime.tv_usec*0.000001);
  double endTS = ((double)endTime.tv_sec)+((double)endTime.tv_usec*0.000001);
  fprintf(stderr,"\x1b[1;32mSucessfully compiled binary with size of %li bytes (%f secs elapsed)\x1b[0m\n",fileSize,endTS-startTS);
  return 0;
}