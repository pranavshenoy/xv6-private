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
} rb_arr[MAX_R_BUFS];


//TODO: need a lock for rbuf


//return 0 if name is valid, -1 if not.
int validate_name(char* name) {
  return 0;
}

// Returns the index of rb_arr. if exists is true, the return index is a valid one.
// Or else, this is the index where the new entry must be done.
// -1 indicated the buffer is full 

int find(char* name, bool* exists) {
  int free_index = -1;
  *exists = false;
  for(int i=0;i<MAX_R_BUFS;i++) {                                                                                                                                                                         
    if(strncmp(name, rb_arr[i].name, strlen(name)) == 0) {
      printf("%s and %s matched\n", name, rb_arr[i].name);
      *exists = true;
      return i;
    }
    if(rb_arr[i].refcount == 0) {
      free_index = i;
    }
  }
  return free_index;
}



uint64
create_ringbuf(char* name, uint64  vm_addr) {

  if(validate_name(name) != 0) {
    printf("Ring buffer name is not valid\n");
    return -1;
  }
  bool exists = false;
  int rbuf_index = find(name, &exists);
  if(rbuf_index == -1) {
    printf("No free ring buffers are available\n");
    return -1;
  }
  if(!exists) {
    printf("Received a free index: %d for name: %s\n", rbuf_index, name);
  } else {
    printf("Received a valid index: %d for name: %s\n", rbuf_index, name);
  }



/*    
  for(int i=0;i<MAX_R_BUFS;i++) {
    printf("first value in rbuf %s \n", rb_arr[i].name);
  }
*/  
  //struct proc *p = myproc();

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



