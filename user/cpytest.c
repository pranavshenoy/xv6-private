#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "stdbool.h"
#include <stdint.h>

char* randomcharchunk(uint64 size);

int
strncmp(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}


bool
compare(char* a, char* b, int size) {

  if(strncmp(a, b, size) == 0) {
    return true;
  }
  return false;
}

void
my_memcpy(void* dst, void* src, uint64  n) {

  typedef uint64 __attribute__((__may_alias__)) u64;
  unsigned char* s =  src;
  unsigned char* d =  dst;

  while((uintptr_t)s%8 != 0 && n != 0) {
    *d++ = *s++;
    n--;
  }
  while(n>=32) {
    *(u64*)d = *(u64*)s;
    d += 8;
    s += 8;
    n -= 8;
    *(u64*)d = *(u64*)s;
    d += 8;
    s += 8;
    n -= 8;
    *(u64*)d = *(u64*)s;
    d += 8;
    s += 8;
    n -= 8;
    *(u64*)d = *(u64*)s;
    d += 8;
    s += 8;
    n -= 8;
  }
  while(n>=8) {
    *(u64*)d = *(u64*)s;
    d += 8;
    s += 8;
    n -= 8;
  }
  while(n>0) {
    *d++ = *s++;
    n--;
  }
}

int testmemcpy(void* dst, void* src, int n, int times) {

  //int size = 4000;
  //int times = 1000000;
  //char* s = randomcharchunk(size);
  //char* d = (char*) malloc(sizeof(char)*size);

  int start = uptime();
  for(int i=0;i<times;i++) {
    my_memcpy(dst, src, n);
    if(!compare(dst, src, n)) {
      printf("Corrupt data!\n");
    }
  }
  int end = uptime();
  return end-start;
  /*
  int start = uptime();
  for(int i=0;i<times;i++) {
    my_memcpy(dst, src, n);
    if(!compare(dst, src, n)) {
      printf("Corrupt data!\n");
    }
  }
  int end = uptime();
  return end-start;
  */
}

int testmemmove(void* dst, void* src, int n, int times) {
  int start = uptime();
  for(int i=0;i<times;i++) {
    memmove(dst, src, n);
    if(!compare(dst, src, n)) {
      printf("Corrupt data!\n");
    }
  }
  int end = uptime();
  return end-start;
}

//Source: https://github.com/joonlim/xv6/blob/master/random.c                                                                                                                                              
// Return a integer between 0 and ((2^32 - 1) / 2), which is 2147483647.                                                                                                                                   

int
random(void)
{
  // Take from http://stackoverflow.com/questions/1167253/implementation-of-rand                                                                                                                          

  static unsigned int z1 = 12345, z2 = 12345, z3 = 12345, z4 = 12345;
  unsigned int b;
  b  = ((z1 << 6) ^ z1) >> 13;
  z1 = ((z1 & 4294967294U) << 18) ^ b;
  b  = ((z2 << 2) ^ z2) >> 27;
  z2 = ((z2 & 4294967288U) << 2) ^ b;
  b  = ((z3 << 13) ^ z3) >> 21;
  z3 = ((z3 & 4294967280U) << 7) ^ b;
  b  = ((z4 << 3) ^ z4) >> 12;
  z4 = ((z4 & 4294967168U) << 13) ^ b;

  return (z1 ^ z2 ^ z3 ^ z4) / 2;
}

// Return a random integer between a given range.                                                                                                                                                          

int
randomrange(int lo, int hi)
{
  if (hi < lo) {
    int tmp = lo;
    lo = hi;
    hi = tmp;
  }
  int range = hi - lo + 1;
  return random() % (range) + lo;
}

//End of source code from https://github.com/joonlim/xv6/blob/master/random.c       

char*
randomcharchunk(uint64 size) {

  char* chunk = malloc(size*sizeof(char));
  for(int i=0;i<size;i++) {
    chunk[i] = randomrange(0, 255);
  }
  return chunk;
}

/*
int
strncmp(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}


bool
compare(char* a, char* b, int size) {

  if(strncmp(a, b, size) == 0) {
    return true;
  }
  return false;
}
*/

int
test(int option, void* dst, void* src, int n) {

  int start, end;
  if(option == 1) {
    start = uptime();
    my_memcpy(dst, src, n);
    end = uptime();
  } else {
    start = uptime();
    memmove(dst, src, n);                                                                        
    end = uptime();
  }
  return end-start;
}


int main() {
  //test memcpy
  int size = 4000;
  int times = 1000000;
  char* src = randomcharchunk(size);
  char* dst = (char*) malloc(sizeof(char)*size);
  //src[40] = '\0';
  //dst[40] = '\0';
  printf("Testing memmove\n");
  int time = testmemmove(dst, src, size, times);
  printf("Time taken: for memmove: %d\n", time);

  time = testmemcpy(dst, src, size, times);
  //printf("dst: %s, src: %s\n", dst, src);
  printf("Time taken: for memcpy: %d\n", time);
  return 0;
}

