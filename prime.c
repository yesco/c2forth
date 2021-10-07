#include <stdio.h>

int isPrime(int n) {
  for(int i=3; i<n; i+=2) {
    int q= n/i;
    int r= n%i;
    if (r==0) return 0;
    if (q<i) return i;
  }
  return 1;
}
  
// 3.5s instead of 3.7 in O3
int isPrimeN(int n) {
  int q= sqrt(n);
  for(int i=3; i<q; i+=2) {
    if (n%i==0) return 0;
  }
  return 1;
}

int bench(int n) {
  int c=4;
  for(int i=11; i<=n; i+=2)
    if (isPrime(i)) c++;
    //if (isPrimeN(i)) c++;
  return c;
}

int main(void) {
//  bench(100);
//  bench(1000);
//  bench(10000);
//  bench(100000);
//  bench(1000000);
//  bench(10000000);
//  printf("%d\n", bench(100000000)); // 100M
//  printf("%d\n", bench(10000000)); // 3.7s O3
  printf("%d\n", bench(10000000)); // 3.7s O3
}
