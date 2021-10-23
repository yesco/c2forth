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

void NIY(char *s) {
  printf("%s\n", s);
  assert(!"error");
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
#define GLOBAL -1

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
  v->rel= GLOBAL;

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

void dumpVars() {
  // dump variables
  fflush(stdout);
  fprintf(stderr, "\n");
  for(int i=0; i<nvar; i++) {
    struct variable *v= &variables[i];
    fprintf(stderr, "VAR %s : %s %s %d\n", v->name,
           !v->type? "SCOPE" : types[v->type<0?-v->type:v->type],
           v->type<0? "*": " ",
           v->rel);
  }
}

// TODO: this doesn NOT handle nested functions, not needed for standard C
void beginScope() {
  addGlobal(0, NULL);
dumpVars();
}

void endScope() {
  int i= nvar;
  // delete all locals added since beginScope
  while(i-->0) {
    struct variable *v= &variables[i];
    if (!v->type) { // is beginscope
      nvar= i;
dumpVars();
      return;
    }
  }
  // reach top scope, reset relative
  lrel= 0;
dumpVars();
}

struct variable *findVar(char* name) {
  for(int i=nvar-1; i>=0; i--) {
    struct variable *v= &variables[i];
    if (v->name && 0==strcmp(v->name, name)) {
      // found
      return v;
    }
  }
  return NULL;
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

void deref(struct variable *v) {
  if (!v) return;

  if (v->rel==GLOBAL) {
    printf(" @ ");
  } else {
    printf(" @frame ");
  }
}

void takeblock();

void takeexpression() {
  int paren= got("(");

  const char *prefixop= getop();

  struct variable *var= NULL;

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
      var= findVar(name);
      if (!var) {
        printf("%% variable '%s' not found\n", name);
        exit(1);
      }
      if (var->rel==GLOBAL) {
        printf("    _%s", name);
      } else {
        printf("    %d ", var->rel, name);
      }
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

  // handle assignment/reference
  const char *op= getop();
  if (op) {
    // assignment
    assert("="=="=");
    if (op=="=") { // EQ, lol
      // "lvalue"
      if (!var) NIY("Can't handle nonvar assignements yet");
      if (var->rel==GLOBAL) {
        op= ":=";
      } else {
        op= ":frame=";
      }
    } else {
      deref(var);
    }
    takeexpression();
    printf("    %s", op);

  } else {
    // just reference
    deref(var);
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
      addLocal(type, name);
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
  beginScope();
  while(takestatement());
  endScope();
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
        beginScope();

        // params
        printf(" {");
        int type;
        while((type= gettype())) {
          char *param= getname();
          if (!param || !*param) break;
          printf("  _%s", param);
          addLocal(type, param);
          free(param);
          got(",");
        }
        printf(" }\n");
        expect(")");

        // body
        takeblock();

        endScope();
        printf("    ;\n");

      } else { // variable
        // TODO: store type... (maybe no need for int and char* lol?)
        // TODO: this is fine for globals, but scopped, local vars in functions?
        addGlobal(type, name);
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

  dumpVars();
}
