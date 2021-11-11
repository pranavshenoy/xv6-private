#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

unsigned long rdcycle(void) {
  unsigned long dst;
  asm volatile ("csrrs %0, 0xc00, x0" : "=r" (dst));
  return dst;
}

int
cyclewrite(int user_src, uint64 src, int n, int minor) {

  /*
  int i;
  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    //uartputc(c);
  }
  */
  return -1;
}

int
cycleread(int user_dst, uint64 dst, int n, int minor) {

  if(n != 8) {
    printf("number of bytes is not 8!\n");
    return -1;
  }
  unsigned long cycle = rdcycle();
  if(either_copyout(user_dst, dst, &cycle, 8) == -1) {
    printf("unable to copy data to user process\n");
    return -1;
  }
  return 0;
}

void
cycleinit(void)
{
  printf("rdcycle initialized\n"); 
  devsw[DEV_CYCLE].read = cycleread;
  devsw[DEV_CYCLE].write = cyclewrite;
}
