#include<stdbool.h>
#include "ringbuf.h"
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"


struct ringbuf {
  int refcount;
  char name[R_BUF_NAME_SIZE];
  void *pa[R_BUF_SIZE];
};

uint64
create_ringbuf(char* name, uint64  vm_addr) {

  struct proc *p = myproc();

/*
  char value = 'g';
  printf("Hello %s!! and  address %p\n", name, vm_addr);
  
  if(copyout(p->pagetable, vm_addr, &value,
                          sizeof(char)) < 0) {
    printf("successfully transfered the data to user space. %p \n", &value);
  } else {
    printf("failure in transferring the data to user space. %p \n", &value);
  }
*/
  return 0;

}



