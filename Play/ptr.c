// https://gist.github.com/yesco/85e6a18195e4def0822c86dae0c82359

#include <stdio.h>
#include <stdlib.h>

typedef char (*FCC)(char a, char b);
typedef int (*FII)(int a, int b);
typedef int (*FLL)(long a, long b);
typedef char* (*FPP)(char *a, char *b);

char fcc(char a, char b) {
  return a+b;
}

int fii(int a, int b) {
  return a+b;
}

long fll(long a, long b) {
  return a+b;
}

char *fpp(char *a, char *b) {
  char *r= malloc(strlen(a)+strlen(b)+1);
  strcpy(r, a);
  strcat(r, b);
  return r;
}

void foid(int a, int b) {
}

int main(void) {
  printf("char=%ld\n", sizeof(char));
  printf("int=%ld\n", sizeof(int));
  printf("long=%ld\n", sizeof(long));
  printf("ptr=%ld\n", sizeof(void*));
  printf("\n");

  printf("fcc=%d\n", fcc(65, 1));
  printf("fii=%d\n", fii(65, 1));
  printf("fll=%d\n", fll(65, 1));
  printf("fpp=%s\n", fpp("foo", "bar"));
  // this line probably won't compile, so commented out
  //printf("foid=%d\t%d\n", foid(99, 72));
  printf("\n");

  FPP f= fcc, v=foid;
  printf("fcc=%d\t%d\n", f(65, 1), 9);
  printf("fii=%d\t%d\n", fii(65, 1), 999);
  printf("fll=%d\t%d\n", fll(65, 1), 9999);
  printf("fpp=%s\t%d\n", fpp("foo", "bar"), 99999);
  printf("foid=%d\t%d\n", v(99, 72), 999999);
}

