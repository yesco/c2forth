//  ALF (ALphabetical Forth)
// 
//  (>) 2021 Jonas SKarlsson
//
// The Conceptual ALfabetical Forth:
// - https://github.com/yesco/ALForth/blob/main/alf-bytecode.txt

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ctype.h>
#include <assert.h>


// TODO:
// - extract symbols from libraries?
// nm -gD /data/data/com.termux/files/usr/lib/* | grep "T strdup


const int trace= 0;


typedef unsigned char byte;
typedef int word;
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
  // put a 0 on stack => quit!
  *sp++= 0;
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

void emit(int c) {
  putchar(c);
  fflush(stdout);
}

int type(byte *s) {
  byte *start= s;
  while(s && *s && *s!='"') {
    if (*s=='\\') s++;
    emit(*s++);
  }
  return s-start;
}

void NIY(char *name) {
  printf("%% %s not implemented yet!\n", name);
  exit(1);
}

void seek(byte end) {
  int n= 0;
  while(pc && pc<MEM) {
    byte c= mem[pc++];
    switch(c) {
    case '(': case '[': case '{':
      n++; break;
    case ')': case ']': case '}':
      if (!n--) return; else break;
    case '"':
      while(pc && pc<MEM) {
        c= mem[pc++];
        if (!c || c=='"') return;
        if (c=='\\') pc++;
      }
    default: break;
    }
  }
  // error
  abort_("Unmatched char in seek");
}

void hash() {
  cell a;

  byte op= mem[pc++];
  if (trace) print_op(pc-1, op);
  switch(op) {

  case '#': a= sp-&memw[RP_INIT/WZ]; *++sp=a; break;
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

void *funs[]={
  exit, malloc, free, realloc,
  fopen, fclose, open, close, lseek, fseek, read, write,
  getc, fgetc, putchar, fputc, printf, sprintf, fprintf, sscanf, fscanf, fgets, fputs,
};

#define LEN(arr) (sizeof(arr)/sizeof(*arr))

void execute() {
  int fn= *sp--;
  assert(fn>0 && fn<LEN(funs));
  cell (*f)(cell, ...) = funs[fn];
  int n= *sp--;
  assert(n>=0 && n<=10); // TODO: ?
  sp-= n; // point to first
  // TODO: siwtch of exact
  cell r= f(sp[0], sp[1], sp[2], sp[3], sp[4], sp[5], sp[6], sp[7], sp[8], sp[9]);
  if (n) sp--;
  *++sp= r;
}

int sum= 0;
void run(int steps) {
  cell a;

// this is pretty optimal now
//#define SUPERSPEED
#ifdef SUPERSPEED
//  while(steps-- && pc) {
//  while(pc) { // save 15 % !

//  while(steps--) { // test is faster than no tes!?

// using this reduces cost 5%
  cell *rp1= rp-1; 

  while(pc) { // test is faster than no tes!?
//  while(1) { // really slow? WTF?

    byte op= mem[pc++];
    // user ops

    // basic ops
    switch(op) {

    case ')': {
      sum++; // cost 2/50
      if ((*rp)-- > 0) {
// really super costly!!!!
        pc= *(rp-1);
//pc= *rp1; // 4 % faster
      } else {
        rp-= 2;
//rp1=rp-1;
      }
      break;
    }

    case '(':
      a= *sp--;
      if (a-->0) {
        *++rp= pc;
        *++rp= a;
//rp1=rp-1;
      } else {
        seek(')');
      }
      break;

    case '0'...'9': literal(op); break; 

      // slightly faster than test before!
    case (128)...(255):
      continue;

    // actually 15% faster here!
    case 0: pc= *rp--;
      if (!pc) return; else break;
      
    default: break;
    }

  }
  return;
#endif

//  while(steps-- && pc) { // slowest 5.37s
//  while(1) {  // slower!!!??? // 5.31s
  while(pc) { // 50% faster! 2.22s  !!!
    byte op= mem[pc++];

    if (trace) print_op(pc-1, op);

    // basic ops
    switch(op) {

    case ')':
// TODO: just for testing
      sum++; // 15% overhead for empty loop
      if ((*rp)-- > 0) {
        pc= *(rp-1);
      } else {
        rp-= 2;
      }
      break;
    case '(':
      a= *sp--;
      if (a-->0) {
        *++rp= pc;
        *++rp= a;
      } else {
        seek(')');
      }
      break;
    case 'i': *++sp= *rp; break;
    case 'j': *++sp= *(rp-2); break;

      // HMMM, control codes?
    case (1)...(9): pc+= op; break; // rel
    case (14)...(31): pc+= op-32; break; // -rel

    case '\\': sp--; break;
    case 'd': a= *sp++; *sp= a; break;
    case 'o': a= *(sp++-1); *sp= a; break;
    case 's': a= *(sp-1); *(sp-1)= *sp; *sp= a; break;
    case 'p': NIY("pick"); break;
    case 'r': rrr(); break;

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
    case '.': printf("%d ", *sp--); fflush(stdout); break;
    case 't': type(&mem[*sp--]); break;

    case 'q': quit(); break;

    case '\n': case '\r': case ' ': break;

    case 'a': allot(*sp--); break;
    case 'h': *++sp= here; break;
      // correct? (if at 1-3?)
    case ',': memw[here+=WZ]= *sp--; break;

    case '?': *sp= !!*sp; break; // if
    case ']': // unloop/exit/leave
    case '[': // again? continue/next

      // 2% faster here than at top
    case 0: pc= *rp--; // rts
      if (pc) continue; else return;

    case 'x': execute(); break;

      // user ops (2% faster below 0!)
    case (128)...(255):
      continue;

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

//    case '\'': // char
//    case '"': // string
//    case ':': // define
//    case ';': // end

//    case 'k': // key()
    default:
      printf("%%Illegal op "); print_op(pc-1, op);
      pc= 0;
      return;
    }
  }
}

int main(void) {
  init(MEM);

  const int bench= 0;
  if (bench) {
    // reference
    if(1) {
      int sum= 0;
      // 1.53s
      for(int i=500000000; i; i--) {
        sum++;
        // % 1000 is 1.35 s and faster???
        //if (i%1000==0) fprintf(stdout, ".");
        if (i%100000000==0) sum++;
      }
      printf("%d\n", sum);
      exit(1);
    } else {
      // 2.24s - 46% overhead (ok...)
      strcomma("500000000()"); 
    }
  } else {
    strcomma("33 44 + .  3(i.)   0(i.) 1(i.)     33 . 500000000() 99 .");
  }
  
  while(pc) run(1024);
  printf("SUM=%d\n", sum);


  free(mem);
}
