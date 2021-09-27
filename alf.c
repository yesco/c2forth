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

word pc, here;
word *sp, *rp;

// memory map
const word ZP_INIT= 0x000; // globals
const word SP_INIT= 0x100; // data stack
const word RP_INIT= 0x200; // return stack
const word PC_INIT= 0x300; // program

void quit() {
  pc= PC_INIT;
  rp= &memw[RP_INIT/WZ];
}

void init(int size) {
  assert(sizeof(cell)==4);
  memw= (void*)(mem= calloc(MEM, 1));
  
  quit();
  here= pc;
  sp= &memw[SP_INIT/WZ];
}

void abort_(char *s) {
  printf("%%Aborted: %s\n", s);
  quit();
}

void OOM() {
  // TODO: longjmp?
  abort_("Out of memory (here>=MEM)");
}

// s, - append  string at here, update here 
word allot(word size) {
  word r= here;
  if (here+size>=MEM) {OOM(); return -1; }
  here+= size;
  return r;
}

void strcomma(char *s) {
  word l= here + strlen(s)+1;
  word t= allot(l);
  if (t>=0) strcpy((char*)&mem[t], s);
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

void NIY(char *name) {
  printf("%% %name not implemented yet!\n", name);
  exit(1);
}

void hash() {
  byte op= mem[pc++];
  if (trace) print_op(pc-1, op);
  switch(op) {

  case '#': *++sp= sp-&memw[RP_INIT/WZ]; break;
  case '-': literal(op); *sp= -*sp; break;

  case 'w': // within

  case 'a': // abs
  case 's': // sign
  case 'x': // max
  case 'm': // min
  case 'q': // sqrt

  case '{': // lshift
  case '}': // rshift

  //case '#/#': // /%

  default:
    printf("%% illegal extened op '#%c'\n", op);
    exit(1);
  }
}

void ccc() {
  cell a;

  byte op= mem[pc++];
  if (trace) print_op(pc-1, op);
  switch(op) {

  case '@': *sp= mem[*sp]; break;
  case '!': a= *sp--; mem[a]= *sp--; break;
  case ',': mem[here++]= *sp--; break;

  case 'r': emit('\r'); break;

  case 'b': a= *sp--; *sp++= ' '; *sp= a; // fallthrough
  case '#':
    for(int n=*sp--; n>0; n--)
      emit(*sp);
    sp--;
    break;

  case '"': // counted string?
  case 'o': // count (string?)
  case '\'':  // c' or '
  case 'c':  // create

  default:
    printf("%% illegal extened op 'c%c'\n", op);
    exit(1);
  }
}

void dollar() {
  byte op= mem[pc++];
  if (trace) print_op(pc-1, op);
  switch(op) {

  case '.': // print hex
  case '"': // pascal string

  case 'l': // accept/readline

  default:
    printf("%% illegal extened op '$%c'\n", op);
    exit(1);
  }
}

void rrr() {
  byte op= mem[pc++];
  if (trace) print_op(pc-1, op);
  switch(op) {

  case '<': *++rp= *sp--; break;
  case '>': *++sp= *rp--; break;

  case 't': // rot
  case '-': // rot-
  case 'l': // roll

  case '@': // r@
  case '!': // r!

  //case 'c': // recurse
  //case 't': // throw
  //case 'e': // err catch

    // r\d<    nr>
    // r\d>    nr>

  default:
    printf("%% illegal extened op 'r%c'\n", op);
    exit(1);
  }
}

// aaa
/*
Allocation:
    al   allot
    
M   am   aallocate (malloc)
M   af   free
M   ar   resize
A   az   size
A   a,   heap compile (steal 2 bytes)
    
A   ac   alloc chain (push on linked list)
    ad   alloc dictionary (create)

    ab   abort
    ab"  abort"

A   ak   assoc
A   ax   assoc execute
    
    ap   pad
*/

void run(int steps) {
  cell a;

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
      // HMMM, control codes?
    case (1)...(9): pc+= op; break; // rel
    case (14)...(31): pc+= op-32; break; // -rel

    case '\\': sp--; break;
    case 'd': a= *sp++; *sp= a; break;
    case 'o': a= *(sp++-1); *sp= a; break;
    case 's': a= *(sp-1); *(sp-1)= *sp; *sp= a; break;
    case 'p': NIY("pick"); break;
    case 'r': rrr(); break;
    case 'i': *++sp= *rp; break;
    case 'j': *++sp= *(rp-1); break;

    case '@': *sp= memw[*sp]; break;
    case '!': a= *sp--; memw[a]= *sp--; break;
      
    case '+': a= *sp--; *sp+= a; break;
    case '-': a= *sp--; *sp-= a; break;
    case '*': a= *sp--; *sp*= a; break;
    case '/': a= *sp--; *sp/= a; break;
    case 'n': *sp= -*sp; break;

      // %% %/ %#  ???
    case '%': a= *sp--; *sp%= a; break;
    case '&': a= *sp--; *sp&= a; break;
    case '|': a= *sp--; *sp|= a; break;
    case '^': a= *sp--; *sp^= a; break;
    case '~': *sp^= 0xff; break;

    case '<': a= *sp--; *sp= a<*sp; break;
    case '>': a= *sp--; *sp= a>*sp; break;
    case '=': a= *sp--; *sp= a=*sp; break;
    case 'z': *sp= !*sp; break;

    case '0'...'9': literal(op); break; 

    case '#': hash(); break;
    case '$': dollar(); break;

    case 'e': emit(*sp--); break;
    case '.': printf("%d ", *sp--); break;
    case 't': type(&mem[*sp--]); break;

    case 'q': quit(); break;

    case '\n': case '\r': case ' ': break;

    case 'a': allot(*sp--); break;
    case 'h': *++sp= here; break;
      // correct? (if at 1-3?)
    case ',': memw[here+=WZ]= *sp--; break;


      // TODO:

//    case '_': break;
//    case '`': break;
//    case '{': break;
//    case '}': break;

//    case 'l': lll(); break;
//    case 'b': bbb(); break;
//    case 'c': ccc(); break;
//    case 'f': fff(); break;
//    case 'g': ggg(); break;
//    case 'l': lll(); break;
//    case 'm': mmm(); break;
//    case 'u': uuu(); break;
//    case 'v': vvv(); break;
//    case 'w': www(); break;
//    case 'y': yyy(); break;

    case '\'': // char
    case '"': // string
    case ':': // define
    case ';': // end

    case '(': // for
    case ')': // loop
    case '?': // if
    case ']': // unloop/exit/leave
    case '[': // again? continue/next

    case 'x': // eval/execute
    case 'k': // key()
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
  strcomma("33 44 + .");

  while(pc) run(1000);

  free(mem);
}
