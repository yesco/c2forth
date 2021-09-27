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

word pc;
word *sp, *rp;

// memory map
const word ZP_INIT= 0x000; // globals
const word SP_INIT= 0x100; // data stack
const word RP_INIT= 0x200; // return stack
const word PC_INIT= 0x300; // program

void init(int size) {
  assert(sizeof(cell)==4);
  memw= (void*)(mem= calloc(MEM, 1));
  
  pc= PC_INIT;
  sp= &memw[SP_INIT/WZ];
  rp= &memw[RP_INIT/WZ];
}

const trace= 1;

void print_op(word addr, byte op) {
  printf("@%4x = '%c' (%02x)\n", addr, (op>31 && op<127)?op:'?', op);
}


void literal(byte op) {
  *++sp= 0;
  while(1) {
    op= op-'0';
    if (op>9) break;
    if (*sp && trace) print_op(pc-1, op+'0');
    *sp= *sp * 10 + op;
    op= mem[pc++];
  }
  pc--;
}

#define emit putchar

int type(byte *s) {
  byte *start= s;
  while(s && *s && *s!='"') {
    if (*s=='\\') s++;
    emit(*s++);
  }
  return s-start;
}

void run(int steps) {
  cell a, b;

  while(steps-- && pc) {
    byte op= mem[pc++];
    if (trace) print_op(pc-1, op);

    // user ops
    if (op & 0x80) {
      
      continue;
    }

    // basic ops
    switch(op) {
    case 0: pc= *sp--; break; // rts
    case (1)...(15): pc+= op; break; // rel
    case (16)...(31): pc+= op-32; break; // -rel

    case '\\': sp--; break;
    case 'd': a= *sp++; *sp= a; break;
    case 'o': a= *(sp++-1); *sp= a; break;

    case '+': a= *sp--; *sp+= a; break;
    case '-': a= *sp--; *sp-= a; break;
    case '*': a= *sp--; *sp*= a; break;
    case '/': a= *sp--; *sp/= a; break;

    case '%': a= *sp--; *sp%= a; break;
    case '&': a= *sp--; *sp&= a; break;
    case '|': a= *sp--; *sp|= a; break;
    case '^': a= *sp--; *sp^= a; break;
    case '~': *sp^= 0xff; break;

    case '0'...'9': literal(op); break; 

    case 'e': emit(*sp--); break;
    case '.': printf("%d ", *sp--); break;
    case 't': type(&mem[*sp--]); break;

    case ' ': break;

    default:
      printf("%%Illegal op "); print_op(mem-1, op);
      pc= 0;
      return;
    }
  }
}

int main(void) {
  init(MEM);

  *sp++= 0;
  strcpy((char*)&mem[pc], "33 44 + .");

  while(pc) run(1000);

  free(mem);
}
