//  ALF (ALphabetical Forth)
// 
//  (>) 2021 Jonas SKarlsson
//
// The Conceptual ALfabetical Forth:
// - https://github.com/yesco/ALForth/blob/main/alf-bytecode.txt

// TODO: test if stack grow up/down is faster...

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
//typedef int word;
//typedef word cell;
typedef long word;
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
  //assert(sizeof(cell)==4);
  assert(sizeof(cell)==sizeof(cell*));
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
  if (t>=0) strcat((char*)&mem[t], s);
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

void niy(char m, char o, char *s) {
  if (!m && !o) {
    printf("%% %s not implemented yet!\n", s);
  } else {
    printf("%% %s '%c %c' not implemented yet!\n", s?s:"", m, o);
  }
  exit(1);
}

void NIY(char *name) {
  niy(0, 0, name);
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

  default: niy('#', op, NULL);
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

  case 'b': a= *sp--; *sp++= ' '; *sp= a;
    // fallthrough // cb char-blanks...
  case '#':
    for(int n=*sp--; n>0; n--)
      emit(*sp);
    sp--;
    break;

  case '"': // counted string?
    // TODO: Not clear how to point and interact with like puts and printf??? maybe need 8 byte proxy pointers?
    // maybe let cell default to pointer size?
    break;
  case 'o': // count (string?)
  case '\'':  // c' or '
  case 'c':  // create

  default: niy('c', op, NULL);
  }
}

void dollar() {
  byte op= mem[pc++];
  if (trace) print_op(pc-1, op);
  switch(op) {

  case '.': // print hex
  case '"': // pascal string

  case 'l': // accept/readline

  default: niy('$', op, NULL);
  }
}

// FramePointer: if "parameters" activated
// TODO: it's actually not a normal FramePointer
//     as normally fp+o == parameter
//                 fp-o == local

// : foo 3 l# lc lb la ;

// 11 22 33 44 foo 
// => 11 44 33 22
cell *fp= NULL;

void lll() {
  cell *a;
  byte op= mem[pc++];
  if (trace) print_op(pc-1, op);
  switch(op) {

    // test: clang alf.c -o alf && (echo "11 22 33 44 2 l# la . lb . la lb + lx . . . " | ./alf) 
  case '#': a= fp; fp= sp-*sp; *++sp= (cell)a; *++sp= (cell)fp; break;
  case 'x': if (fp) {
    a= sp;
    while(*sp--!=(cell)fp);
    int n= a-sp-1;
    // replace args with return vals
    memmove(fp, a, n*sizeof(cell));
    a= fp;
    fp= (cell*)*sp--; // restore prev
    sp= a+n-1;
    printf(" {%d} ", n);
    break; }
  case 'a'...'j': *++sp= *(fp+(op-'a')); break;

  default: niy('l', op, NULL);
  }
}

void user(byte op) {
  NIY("user()");
}

void rrr() {
  cell a;

  byte op= mem[pc++];
  if (trace) print_op(pc-1, op);
  switch(op) {

  case '<': *++rp= *sp--; break;
  case '>': *++sp= *rp--; break;

  case 't': // rot: 2 1 0 -> 1 0 2
    a= sp[0]; sp[0]= sp[-2]; sp[-2]= sp[-1]; sp[-1]= a; break;
    // -rot 2 1 0 -> 0 2 1
  case '-': NIY("-rot"); // -rot
  case 'l': NIY("roll"); // roll

  case '@': // r@
  case '!': // r!

  //case 'c': // recurse
  //case 't': // throw
  //case 'e': // err catch

  // r\d<    nr>
  // r\d>    nr>

  default: niy('r', op, NULL);
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

// https://sourceware.org/newlib/libm.html
// 
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
  int n= *sp;
  assert(n>=0 && n<=10); // TODO: ?
  sp-= n; // point to first
  // TODO: siwtch of exact
  if (!n) sp++;
  sp[0] = f(sp[0], sp[1], sp[2], sp[3], sp[4], sp[5], sp[6], sp[7], sp[8], sp[9]);
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
    case (1)...(7): pc+= op; break; // rel
    case (14)...(31): pc+= op-32; break; // -rel

    case '\\': sp--; break;
    case 'd': a= *sp++; *sp= a; break;
    case 'o': a= *(sp++-1); *sp= a; break;
    case 's': a= *(sp-1); *(sp-1)= *sp; *sp= a; break;
    case 'p': NIY("pick"); break;
    case 'r': rrr(); break;
    case 'l': lll(); break;

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

    case '\n': case '\r': case ' ': case '\t': break;

    case 'a': allot(*sp--); break;
    case 'h': *++sp= here; break;
      // correct? (if at 1-3?)
    case ',': memw[here+=WZ]= *sp--; break;

    case '?': *sp= !!*sp; break; // if
    case ']': NIY("exit"); // unloop/exit/leave
    case '[': NIY("next"); // again? continue/next

      // 2% faster here than at top
    case 0: pc= *rp--; // rts
      if (pc) continue; else return;

    case 'x': execute(); break;

      // TODO: optimize for space&speed
      // 0-9 x
    // case (128+'0')...(128+'9'):

      // unicode 10xx xxxx (64 cont)
      //   case (0x80)...(0xbf):
      // unicode 110x xxxx (32 2b)
      // unicode 1110 xxxx (16 3b)
      // unicode 1111 0xxx (8 4b)

      // unicode 1111 10xx (4 -)  ?
      // unicode 1111 110x (2 -)  ?
      // unicode 1111 1110 (1) (debug)
      // unicode 1111 1111 (1) (return)

    case '`': // (var= val)=>val
      a= *sp--;
      *sp= *(cell*)*sp= a;
      break;

      // user ops (2% faster below 0!)
    case 'A'...'Z': // temp proc name
      // TODO: make A..Z be params+locals!
      // at end def:
      //
      //     ; 3 4 parlocs

    case (128)...(255): // user dispatch
      user(op); break;

    case '"': // string
      *++sp= MEM+pc; // add of string
      while(pc && pc<MEM) {
        byte c= mem[pc++];
        if (!c || c=='"') break;
        if (c=='\\') pc++;
      }
      break;

      // --- TODO:

//    case '\'': // char
//    case ':': // define
//    case ';': // end

//    case 'k': // key()

//    case '_': break; // ???
//    case '{': break; // local scope?
//    case '}': break;

//    case 'b': bbb(); break; // buffer
//    case 'c': ccc(); break; // char
//    case 'f': fff(); break; // float
//    case 'g': ggg(); break; // geap
//    case 'm': mmm(); break; // memory
//    case 'u': uuu(); break; // unsigned
//    case 'v': vvv(); break; // variable?
//    case 'w': www(); break; // 2...
//    case 'y': yyy(); break; // ???

    default: niy(op, 0, NULL);
    }
  }
}

void test() {
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
    // 65 1 14 x == putchar('A')
    strcomma("33 44 + .  3(i.)   0(i.) 1(i.)     33 . 77 65 1 14 x . .");
  }
  while(pc) run(1024);
  printf("SUM=%d\n", sum);
}

int main(void) {
  init(MEM);
  //test();

  FILE* f= stdin;

  char buf[255]= {0};
  int lineno= 0;

  char *ln;
  while((ln= fgets(buf, sizeof(buf), f))) {
    lineno++;
    if (ln[strlen(ln)-1]=='\n') {
      ln[strlen(ln)-1]= 0;
    }
    if (!*ln) continue;
    
    strcomma(ln);
  }
  // fclose(f);

  while(pc) run(1024);

  free(mem);
}

