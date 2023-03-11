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

typedef enum {
  LABEL,
  INSTRUCTION,
  DATA,
  BSS_EXPAND,
  ALIGNMENT,
  INCLUDE_BIN,
  SECTION,
} TokenType;

typedef struct {
  void* next;
  TokenType type;
} TokenHeader;

typedef enum {
  ARG_NONE,
  ARG_REG,
  ARG_IMM,
  ARG_LOADSTORE,
} ArgType;

struct OkamiOpcode {
  int opcode;
  const char* name;
  ArgType arg1;
  ArgType arg2;
  ArgType arg3;
  ArgType arg4;
};

const char* OkamiRegisters[] = {"zero","a0","a1","a2","a3","a4","a5","a6","a7","t0","t1","t2","t3","t4","t5","t6","t7","s0","s1","s2","s3","s4","s5","s6","s7","s8","s9","gp","kr","fp","sp","ra"};

const struct OkamiOpcode OkamiInstructions[] = {
  /* SECTION 1 */
  { 0,"add",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  { 1,"sub",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  { 2,"and",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  { 3,"or",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  { 4,"xor",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  { 5,"sll",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  { 6,"srl",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  { 7,"sra",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  { 8,"slt",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  { 9,"sltu",ARG_REG,ARG_REG,ARG_REG,ARG_NONE},
  {10,"mul",ARG_REG,ARG_REG,ARG_REG,ARG_REG},
  {11,"mulu",ARG_REG,ARG_REG,ARG_REG,ARG_REG},
  {12,"div",ARG_REG,ARG_REG,ARG_REG,ARG_REG},
  {13,"divu",ARG_REG,ARG_REG,ARG_REG,ARG_REG},
  /* SECTION 2 */
  {16,"addi",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {17,"andi",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {18,"ori",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {19,"xori",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {20,"slli",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {21,"srli",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {22,"srai",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {23,"slti",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {24,"sltiu",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {25,"lui",ARG_REG,ARG_IMM,ARG_NONE,ARG_NONE},
  /* SECTION 3 */
  {32,"b",ARG_IMM,ARG_NONE,ARG_NONE,ARG_NONE},
  {33,"bl",ARG_IMM,ARG_NONE,ARG_NONE,ARG_NONE},
  {34,"blr",ARG_REG,ARG_REG,ARG_NONE,ARG_NONE},
  {35,"beq",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {36,"bne",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {37,"bge",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {38,"blt",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {39,"bgeu",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  {40,"bltu",ARG_REG,ARG_REG,ARG_IMM,ARG_NONE},
  /* SECTION 4 */
  {48,"lb",ARG_REG,ARG_LOADSTORE,ARG_NONE,ARG_NONE},
  {49,"lbu",ARG_REG,ARG_LOADSTORE,ARG_NONE,ARG_NONE},
  {50,"lh",ARG_REG,ARG_LOADSTORE,ARG_NONE,ARG_NONE},
  {51,"lhu",ARG_REG,ARG_LOADSTORE,ARG_NONE,ARG_NONE},
  {52,"lw",ARG_REG,ARG_LOADSTORE,ARG_NONE,ARG_NONE},
  {53,"sb",ARG_REG,ARG_LOADSTORE,ARG_NONE,ARG_NONE},
  {54,"sh",ARG_REG,ARG_LOADSTORE,ARG_NONE,ARG_NONE},
  {55,"sw",ARG_REG,ARG_LOADSTORE,ARG_NONE,ARG_NONE},
  /* EXTENDED REGISTERS */
  {60,"mfex",ARG_REG,ARG_IMM,ARG_NONE,ARG_NONE},
  {61,"mtex",ARG_REG,ARG_IMM,ARG_NONE,ARG_NONE},
  /* TRAPS */
  {62,"kcall",ARG_IMM,ARG_NONE,ARG_NONE,ARG_NONE},
  {62,"mcall",ARG_IMM,ARG_NONE,ARG_NONE,ARG_NONE},
  {63,"rft",ARG_NONE,ARG_NONE,ARG_NONE,ARG_NONE},
  /* PSEUDO-INSTRUCTIONS */
  {-1,"nop",ARG_NONE,ARG_NONE,ARG_NONE,ARG_NONE},
  {-2,"li",ARG_REG,ARG_IMM,ARG_NONE,ARG_NONE},
  {-3,"la",ARG_REG,ARG_IMM,ARG_NONE,ARG_NONE},
  {-4,"br",ARG_REG,ARG_NONE,ARG_NONE,ARG_NONE},
  {-5,"mv",ARG_REG,ARG_REG,ARG_NONE,ARG_NONE}
};

void Error(long line, long col, char *str) {
  fprintf(stderr, "\x1b[31m%li:%li - %s\x1b[0m\n", line, col, str);
  exit(1);
}

int iswhitespace(char val) {
  return val == ' ' || val == '\t' || val == '\r' || val == '\n';
}

void Assemble(char* name, char* data) {
  unsigned long len = strlen(data);
  unsigned long i = 0;
  unsigned long line = 0;
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

      } else if(strncmp(data+start,".text",length) == 0) {

      } else if(strncmp(data+start,".text",length) == 0) {
    }
  }
}

int main(int argc, char **argv) {

  return 0;
}