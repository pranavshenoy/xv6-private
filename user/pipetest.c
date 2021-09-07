#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "stdbool.h"
#include <stdint.h>

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

//From kernel/string.c
int
strncmp(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

uint64
receivebytes(int readfd, char* chunk, int size) {

  int bytesread = 0;
  while(true) {
    int bytes = read(readfd, &chunk[bytesread], size-bytesread);
    if(bytes == -1) {
      return -1;
    }
    if(bytes == 0) {
      break;
    }
    bytesread += bytes;
  }
  return bytesread;
}

uint64
sendbytes(int writefd, char* data, int size) {

  int byteswritten = 0;
  while(true) {
    int bytes = write(writefd, &data[byteswritten], size-byteswritten);
    if(bytes == -1) {
      return -1;
    }
    byteswritten += bytes;
    if(byteswritten == size) {
      break;
    }
  }
  return byteswritten;
}

bool
compare(char* a, char* b, int size) {

  if(strncmp(a, b, size) == 0) {
    return true;
  }
  return false;
}

void
parentprocess(int readfd, char* verifydata, int size) {

  int start = uptime();
  char* chunk = malloc(size*sizeof(char));;
  int bytes = receivebytes(readfd, chunk, size);
  int end = uptime();
  if(bytes != size) {
    printf("Received corrupted data. Size doesn't match.\n");
    return;
  }
  if(!compare(verifydata, chunk, size)) {
    printf("The received data is corrupted. Bytes doesn't match.\n");
    return;
  }
  wait(0);
  printf("Total time taken to finish transfer: %d ticks\n", end - start);
}

void
childprocess(int writeFd, char* data, int size) {
  
  int bytes = sendbytes(writeFd, data, size);
  if(bytes == -1) {
    printf("Couldn't send all the data\n");
  }
}

int
main(int argc, char *argv[])
{
  int p[2];
  pipe(p);
  unsigned long size = 1000*1000*40;                                                                                                                                                                   
  char* data = randomcharchunk(size);  
  int pid = fork();
  if(pid == -1) {
    printf("Some error occured while forking");
    exit(0);
  }
  if(pid == 0) {
    close(p[0]);
    childprocess(p[1], data, size);
    close(p[1]);
  } else {
    close(p[1]);
    parentprocess( p[0], data, size);
    close(p[0]);
  }
  exit(0);
}

