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

int gottype() {
  if (got("void") || got("int") || got("char")) {
    got("*"); // optional
    return 1;
  }
  return 0;
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
      printf("    %s\n", name);
      
    } else {
      // variable
      printf("    %s", name);
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
    takeexpression();
    printf("    %s", op);
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
  } else if (isalnum(peek())) {
    // "proc" call (no care value)
    takeexpression();
    printf("    drop\n");
  } else {
    return 0;
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

    // function definition
    if (gottype()) {
      char *name= getname();
      printf("    : %s ", name);
    
      expect("(");
      printf(" {");
      while(gottype()) {
        char *param= getname();
        if (!param) break;
        printf("  %s", param);
        free(param);
        got(",");
      }
      printf(" }\n");
      expect(")");

      takeblock();

      printf("    ;\n");
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
