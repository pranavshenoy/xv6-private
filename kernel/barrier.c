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

struct barrier {
  int p_count;
  struct spinlock lock;
} b[10];

int
barrierwrite(int user_src, uint64 src, int n, int minor) {
  
  if(n != 4) {
    printf("invalid bytes\n");
    return -1;
  }
  printf("wake-up call for buffer id: %d\n", minor);
  acquire(&b[minor].lock);
  barrier_count(&b[minor], b[minor].p_count);
  b[minor].p_count = 0;
  release(&b[minor].lock);
  wakeup(&b[minor]);
  return 0;
}

int
barrierread(int user_dst, uint64 dst, int n, int minor) {

  if(n != 4) {
    printf("invalid #bytes\n");
    return -1;
  }
  acquire(&b[minor].lock);
  b[minor].p_count++;
  printf("Process going to sleep. Barrier id: %d  \n", minor);
  sleep(&b[minor], &b[minor].lock);
  printf("Process woke up. Barrier id: %d\n", minor);
/*
  if(either_copyout(user_dst, dst, &(myproc()->barrier_wait_count), 4) == -1) {
    myproc()->barrier_wait_count = 0;
    printf("unable to copy data to user process\n");
    release(&b[minor].lock);
    return -1;
  }
*/
  printf("total process waiting: %d\n", myproc()->barrier_wait_count);
  myproc()->barrier_wait_count = 0;
  release(&b[minor].lock);
  return 0;
}

void
barrierinit(void)
{
  for(int i=0;i<9;i++) {
    initlock(&b[i].lock, "barrier");
  }
  printf("initialising barrier\n");
  devsw[DEV_BARRIER].read = barrierread;
  devsw[DEV_BARRIER].write = barrierwrite;
}
