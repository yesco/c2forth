#include <stdio.h>

int a;
int b= 42;
char *s= "foo";

int main(void) {
  printf("a0=%d\n", a);
  a= 7;
  printf("a7=%d\n", a);
  a= a+1;
  printf("a8=%d\n", a);
  //a++;
  //printf("a++=%d\n", a);
  {
    int a=11;
    printf("  a11,b42=%d,%d\n", a, b);
    b= a*2;
    printf("  a11,b22=%d,%d\n", a, b);
    a= b*3;
    printf("  a66,b22=%d,%d\n", a, b);
  }
  printf("a8,b22=%d,%d\n", a, b);
  printf("sfoo=%s\n", s);
  s= "bar";
  printf("sbar=%s\n", s);
  s= s+1;
  printf("sar=%s\n", s);
}