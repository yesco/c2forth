#include <stdio.h>
#include <stdlib.h>

#define TYPE int
// - *2 but *4 in time!
//#define MAX (1024*1024*10*10*2)
#define TOT (1024L*1024L*1024L*100)
//#define MAX (1024*1024*100*1)
//#define MAX (1024*64) // <64 K 5.40s
//#define MAX (128) // too small! 12.7s!
//#define MAX (1024*4) // 5.37!
//#define MAX (1024*8) // 5.24!
//#define MAX (1024*16) // 5.19!
//#define MAX (1024*32) // 5.14!
//#define MAX (4) // 6.8s

// "optimal" (for 2 nested loops)
//#define MAX (1024*32) // 5.14! char

//#define MAX (1024*32) // 11.07 short
//#define MAX (1024*64) // 19s short
//#define MAX (1024*16) // 10.25 short
//#define MAX (1024) // 11s short
//#define MAX (512) // 12s short

//#define MAX (1024) // 21.93 int
#define MAX (1024*16) // 21.61 int ok (64K)
//#define MAX (1024*32) // 38s int 2x
//#define MAX (1024*64) // 40s int 2x

TYPE *data= NULL;

int main(void) {
  data= malloc(sizeof(TYPE)*MAX);
  TYPE *end= &data[MAX];
  TYPE *p;

  p= data;
  int i= 0;
  while (++p<end) {
    *p= i++;
  }

  p= data;
  TYPE sum= 0;
  if (0){
    while (++p<end) {
      sum+= *p;
    } 
  } else {
    for(long i= 0; i<TOT; ) {
      for(long j= 0; j<MAX; j++) {
        sum+= p[j];
        i++;
      }
    }
  }
      
  printf("sum=%d (sizeof=%d)", sum, sizeof(TYPE));
}
