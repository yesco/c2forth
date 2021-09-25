// Minimal C Compiler

#include <stdio.h>
#include <ctype.h>
#include <assert.h>

char line[255]= {0};

char *skipspc(char *s) {
  while(s && *s && isspace(*s)) s++;
  return s;
}

int is(char *s, const char *i) {
  return strncmp(s, i, strlen(i))==0;
}

char *skip(char *s, const char *i) {
  s= skipspc(s);
  if (is(s, i)) s+= strlen(i);
  s= skipspc(s);
  return s;
}

int got(char **p, const char *i) {
  *p= skipspc(*p);
  char *s= skip(*p, i);
  int r= s!=*p;
  *p= skipspc(s);
  return r;
}

char *next(char *s) {
  s= skipspc(s);
  while(*s && !isspace(*s)) s++;
  s= skipspc(s);
  return s;
}

int istype(char *s) {
  return is(s, "int") || is(s, "void");
}

char *getname(char *s) {
  s= skipspc(s);
  char *start= s;
  while (*s && isalnum(*s)) s++;
  return strndup(start, s-start);
}

char *takename(char **p) {
  char *r= getname(*p);
  *p+= strlen(r);
  return r;
}

int skippedtype(char **p) {
  if (istype(*p)) {
    *p= next(*p);
    if (is(*p, "*")) *p= skipspc(*p+1);
    return 1;
  }
  return 0;
}

const char *ops[]= {
  "++", "+", "--", "-", "*", "/", "%", "&&", "&", "||", "|", "^", "<<", "<", ">>", ">", "==", "="};

const char *getop(char **p) {
  for(int i=0; i<sizeof(ops)/sizeof(*ops); i++) {
    if (got(p, ops[i])) return ops[i];
  }
  return NULL;
}

void takeblock(char **p);

void expect(char **p, const char *e) {
  if (!got(p, e)) {
    printf("ERROR AT '%s'\n", *p);
    printf("Expected '%s'\n", e);
    assert(!"unexpected char");
  }
}

void takeexpression(char **p) {
  int paren= got(p, "(");

  const char *prefixop= getop(p);

  if (isalpha(**p)) {
    char *name= takename(p);
  
    // function call? ( , )
    if (is(*p, "(")) {
      do {
        *p= skip(*p, ",");
        takeexpression(p);
      } while (is(*p, ","));
      printf("\t%s\n", name);

    } else {
      // variable
      printf("\t%s\n", name);
    }
    free(name);

  } else if (isdigit(**p)) {
    // number
    int d= 0, r, n= 0;
    if (1== (r= sscanf(*p, "%d %n", &d, &n))) {
      printf("\t%d\n", d);
      *p+= n;
    } else {
      printf("scanf=>r=%d\n", r);
      printf("ERROR AT '%s'\n", *p);
      assert(!"bad char in number");
    }
  } else if (got(p, "\"")) {
    // string
    printf("\t\"");
    while(**p && **p!='"') {
      putchar(**p);
      (*p)++;
    }
    printf("\"\n");
  } else if (got(p, "\'")) {
    printf("\t%d\n", **p);
    assert(got(p, "\'"));
  } else {
    printf("ERROR AT '%s'\n", *p);
    assert(!"unexpected char");
  }

  const char *op= getop(p);
  if (op) {
    takeexpression(p);
    printf("\t%s\n", op);
  }

  if (prefixop)
    printf("\tPREFIX_%s\n", prefixop);

  if (paren) expect(p, ")");
}

int takestatement(char **p) {
  if (is(*p, "{")) {
    takeblock(p);
  } else if (got(p, "return")) {
    takeexpression(p);
  } else if (got(p, "if")) {
    takeexpression(p);
    takestatement(p);
    if (got(p, "else"))
      takestatement(p);
  } else if (got(p, "while")) {
    takeexpression(p);
    // TODO:
  } else if (got(p, "do")) {
    takeexpression(p);
    // TODO:
  } else if (got(p, ";")) {
    ; // haha!
  } else {
    // "proc" call (no care value)
    takeexpression(p);
    printf("\tdrop\n");
  }
}

void takeblock(char **p) {
  expect(p, "{");
  while(takestatement(p));
  expect(p, "}");
}

int main(void) {
  char *ln;
  int n=0;
  while((ln= fgets(line, sizeof(line), stdin)) && *ln) {
    n++;
    if (ln[strlen(ln)-1]=='\n')
      ln[strlen(ln)-1]= 0;
    printf("%d: %s\n", n, line);

    ln= skipspc(ln);
    if (!*ln) continue;
    if (is(ln, "//")) continue;
    if (is(ln, "#include")) continue;

    // function definition
    if (skippedtype(&ln)) {
      char *name= takename(&ln);
      printf("\t: %s \n", name);

      expect(&ln, "(");
      while(skippedtype(&ln)) {
        char *param= takename(&ln);
        printf("\t-- param: %s\n", param);
        free(param);
        (void)got(&ln, ",");
      }
      expect(&ln, ")");

      takeblock(&ln);

      printf("\t;\n");
      free(name);
      continue;
    }

    // over-simplified
    takestatement(&ln);
  }
}
