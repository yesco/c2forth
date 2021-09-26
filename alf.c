#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

typedef unsigned char byte;
typedef unsigned int word;
typedef word cell;

#define WZ (sizeof(word))
#define MEM (4096)
#define MEMW (MEM/WZ)

// two variants of same memory
byte *mem= NULL;
word *memw= NULL;

byte *pc, *lastdigit;
word *sp, *rp;

const word PC_INIT= 0x200;
const word SP_INIT= 0x300/WZ;
const word RP_INIT= 0x400/WZ;

void init(int size) {
  assert(sizeof(cell)==4);
  memw= (void*)(mem= calloc(MEM, 1));
  
  pc= &mem[PC_INIT];
  sp= &memw[SP_INIT];
  rp= &memw[RP_INIT];
  lastdigit= NULL;
}

void run(int steps) {
  cell a, b;

  while(steps--) {
    byte op= *pc++;
    switch(op) {
    case 0: // exit
      return;

    case '\\': sp--; break;
    case 'd': a= *sp++; *sp= a; break;

    case '+': a= *sp--; *sp+= a; break;
    case '-': a= *sp--; *sp-= a; break;
    case '*': a= *sp--; *sp*= a; break;
    case '/': a= *sp--; *sp/= a; break;

    case '%': a= *sp--; *sp%= a; break;
    case '&': a= *sp--; *sp&= a; break;
    case '|': a= *sp--; *sp|= a; break;
    case '^': a= *sp--; *sp^= a; break;
    case '~': *sp^= 0xff; break;

    case '0'...'9':
      if (lastdigit==pc-1) *sp*= 10;
      *sp++= op-'0';
      lastdigit= pc;
      break;
      
    default:
      printf("%%Illegal @%4x = '%c' (%2x)\n", (word)(pc-PC_INIT-1), op, op);
      return;
    }
  }
}

int main(void) {
  init(MEM);

  while(1) run(1000);

  free(mem);
}
