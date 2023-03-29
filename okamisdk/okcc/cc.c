#include <stdio.h>
#include "cc.h"
#include "notice.h"

int main(int argc,char *argv) {
    fputs(VERSION, stderr);
    fputs(CRIGHT, stderr);
    if(argc < 3) {
        fputs("usage: cc [file(s)] [asm_out]\n",stderr);
        return 1;
    }
    
    return 0;
}