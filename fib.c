#include <stdio.h>

int fib(int n) {
  if (n<0) return 0;
  if (n==1) return 1;
  return fib(n-1)+fib(n-2);
}

int main(void) {
  printf("Fib(7)=%d\n", fib(7));
  printf("%d %d% %d %d\n", 1,2,3,4);
}
