// Minimal C Compiler

#include <stdio.h>
#include <ctype.h>
#include <assert.h>

char buf[255]= {0};
int bufn= 0;

FILE *f= NULL;
int c= 0, lineno= 0;

void skipspc();

int is(const char *i);

int ensure() {
  if (bufn>0) return 1;

  bufn= 0;
  char *ln= fgets(buf, sizeof(buf), f);
  if (!ln) return 0;

  bufn= strlen(ln);
  lineno++;
  if (ln[strlen(ln)-1]=='\n') {
    ln[strlen(ln)-1]= 0;
    bufn--;
  }
  printf("\e[32m%d: %s\e[37m\n", lineno, buf);

  skipspc();
  if (!*buf || is("//") || is("#include")) {
    // force another call
    bufn= 0;
  }

  return ensure();
}

int peek() {
  return ensure()? buf[0]: 0;
}

int step() {
  int c= peek();
  memcpy(buf, buf+1, sizeof(buf)-1);
  if (--bufn<0) bufn= 0;
  return c;
}

void skipspc() {
  while(ensure() && isspace(peek()))
    step();
}

int is(const char *i) {
  return ensure() && strncmp(buf, i, strlen(i))==0;
}

int got(const char *i) {
  skipspc();
  if (is(i)) {
    int n= strlen(i);
    while(n--) step();
    return 1;
  }
  return 0;
}

void expect(const char *e) {
  if (!got(e)) {
    printf("ERROR AT %d '%s'\n", bufn, buf);
    printf("Expected '%s'\n", e);
    assert(!"unexpected char");
  }
}

char *getname() {
  skipspc();
  // TODO: may not always work? (nl/comement?)
  char *s= buf;
  while (*s && isalnum(*s)) s++;
  char *r = strndup(buf, s-buf);
  int n= strlen(r);
  while(n--) step();
  return r;
}

const char *types[]= {
  "_DYMMUTYPE", "void", "char", "int", "long", "float", "double", "struct"};

// returns type index, -index if pointer
// (However: "char *a, b" - b is char!)
int gettype() {
  int _static= got("static");
  // TODO: handle static (scoped globals!)
  // (how? may need move locals?)
  assert(!_static);
  int _signed= got("signed");
  int _unsigned= got("unsigned");
  // long long? long double/float?

  for(int i=0; i<sizeof(types)/sizeof(*types); i++) {
    if (got(types[i])) {
      // TODO: structs/union
      assert(types[i]!="struct");
      int pointer= got("*");
      return pointer?-i:i;
    }
  }
  // no type recognized
  // TODO: typedefs?
  if (_signed || _unsigned)
    assert(!"unexpected signed/unsigned");
  return 0;
}


#define MAXVAR 1024

struct variable {
  int type; // 0==new scope
  char *name;
  int rel;
} variables[MAXVAR]= {0};

int nvar= 0;
int lrel= 0;

void addGlobal(int type, char *name) {
  assert(nvar<MAXVAR);

  struct variable *v= &variables[nvar];
  v->type= type;
  v->name= name?strdup(name):NULL;
  v->rel= -1;

  nvar++;
}

void addLocal(int type, char *name) {
  assert(nvar<MAXVAR);

  struct variable *v= &variables[nvar];
  v->type= type;
  v->name= name?strdup(name):NULL;
  v->rel= ++lrel;

  nvar++;
}

// TODO: this doesn NOT handle nested functions, not needed for standard C
void beginScope() {
  addGlobal(0, NULL);
}

void endScope() {
  int i= nvar;
  // delete all locals added since beginScope
  while(i-->0) {
    struct variable *v= &variables[i];
    if (!v->type) { // is beginscope
      nvar= i-1;
      return;
    }
  }
  // reach top scope, reset relative
  lrel= 0;
}

void takeexpression();

void takevardef(int type, char *name, const char *scope) {
  int array= got("[");
  // TODO: handle array decl/assign
  assert(!array);
  printf("    %s _%s\n", scope, name);
  if (got("=")) {
    takeexpression();
    printf("    _%s !\n", name);
  }
}

const char *ops[]= {
  "++", "+", "--", "-", "*", "/", "%", "&&", "&", "||", "|", "^", "<<", "<", ">>", ">", "==", "="};

const char *getop() {
  for(int i=0; i<sizeof(ops)/sizeof(*ops); i++) {
    if (got(ops[i])) return ops[i];
  }
  return NULL;
}

void takeblock();

void takeexpression() {
  int paren= got("(");

  const char *prefixop= getop();

  int isvar= 0;

  if (isalpha(peek())) {
    char *name= getname();
  
    // function call? ( , )
    if (got("(")) {
      // TODO: this is PASCAL calling style
      // C-calling style would be reversed!
      // (for argv/printf it's needed!)
      do {
        takeexpression();
      } while (got(","));
      expect(")");
      printf("    _%s\n", name);
      
    } else {
      // variable (value)
      printf("    _%s", name);
      isvar= 1;
    }
    if (name) free(name);

  } else if (isdigit(peek())) {
    char *s= getname();
    int d= 0, r, n= 0;
    if (1== (r= sscanf(s, "%d %n", &d, &n))) {
      printf("    %d", d);
    } else {
      printf("scanf=>r=%d\n", r);
      printf("ERROR AT %d '%s'\n", bufn, buf);
      assert(!"bad char in number");
    }
    if (s) free(s);
  } else if (got("\"")) {
    // string
    printf("    \"");
    while(peek() && peek()!='"')
      putchar(step());
    step();
    printf("\"\n");
  } else if (got("\'")) {
    printf("    %d\n", step());
    expect("\'");
  } else {
    printf("ERROR AT %d '%s'\n", bufn, buf);
    assert(!"unexpected char");
  }

  const char *op= getop();
  if (op) {
    // LOL, because of hashstrings...
    assert("="=="=");
    if (op=="=") { // EQ, lol
      // "lvalue"
      op= ":=";
    } else if (isvar) {
      printf(" @ ");
    }
    takeexpression();
    printf("    %s", op);
  } else if (isvar) {
    printf(" @ ");
  }

  if (prefixop)
    printf("    PREFIX_%s\n", prefixop);

  if (paren) expect(")");
}

int takestatement() {
  if (is("{")) {
    takeblock();
  } else if (got("return")) {
    takeexpression();
    printf("    exit\n");
  } else if (got("if")) {
    takeexpression();
    printf("    if\n");
    takestatement();
    if (got("else")) {
      printf("    else");
      takestatement();
    }
    printf("    endif\n");
  } else if (got("while")) {
    takeexpression();
    // TODO:
  } else if (got("do")) {
    takeexpression();
    // TODO:
  } else if (got(";")) {
    ; // haha!
  } else {
    int type= gettype();
    if (type) {
      // local var?
      char *name= getname();
      assert(name);
      takevardef(type, name, "local");
      if (name) free(name);
      return 1;
    }

    // proc/func call/assignment
    if (isalnum(peek())) {
      takeexpression();
      // assume all fun/assign return value
      printf("    drop\n");
      // TODO: don't drop if first parameter is same value/var! (do at byte coder?)
      return 1;

    } else {
      // not recognized
      return 0;
    }
  }
  return 1;
}

void takeblock() {
  expect("{");
  while(takestatement());
  expect("}");
}

void takeprogram() {
  while(peek()) {

    // definition func/var
    int type= gettype();
    if (type) {
      char *name= getname();
      if (got("(")) { // function
        printf("    : _%s ", name);
    
        // params
        printf(" {");
        int type;
        while((type= gettype())) {
          char *param= getname();
          if (!param || !*param) break;
          printf("  _%s", param);
          // TODO: capture name assign local id
          free(param);
          got(",");
        }
        printf(" }\n");
        expect(")");

        // body
        takeblock();
        printf("    ;\n");

      } else { // variable
        // TODO: store type... (maybe no need for int and char* lol?)
        // TODO: this is fine for globals, but scopped, local vars in functions?
        takevardef(type, name, "variable");
      }

      if (name) free(name);
    } else {
      
      // over-simplified
      takestatement();
    }
  }
}

int main(void) {
  f= stdin;

  takeprogram();
}
