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
      *exists = true;
      return i;
    }
    if(rb_arr[i].refcount == 0) {
      free_index = i;
    }
  }
  return free_index;
}


int free_pm(int pages, void* phy_addr) {

  for(int i=0;i<pages;i++) {
    kfree(phy_addr+i);
  }
  return 0;
}

int allocate_pm(int pages, void** phy_addr) {
  for(int i=0;i<pages;i++) {
    void* ptr = kalloc();
    printf("Phy mem address after allocation %p\n", ptr);
    *(phy_addr+i) = ptr;
    printf("Phy mem address in pointer %p\n", *(phy_addr+i));
    if(*(phy_addr+i) == 0) {
      free_pm(i, phy_addr);
      return -1;
    }
  }
  return 0;
}

int new_rbuffer(char* name, int rbuf_index) {

  rb_arr[rbuf_index].refcount += 1;
  strncpy(rb_arr[rbuf_index].name, name, strlen(name));
  if(allocate_pm(R_BUF_SIZE, rb_arr[rbuf_index].pa) != 0) {
    return -1;
  }
  return 0;
}


//TESTING
void display_pm(int pages, int rbuf_index) {
  for(int i=0;i<pages;i++) {
    printf("Physical memory allocated %p \n", (uint64) *(rb_arr[rbuf_index].pa+i));
  }
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
    if(new_rbuffer(name, rbuf_index) != 0) {
      printf("unable to allocate physical memory\n");
      return -1;
    }
    printf("Received a free index: %d for name: %s\n", rbuf_index, name);
  } 
  //display_pm(R_BUF_SIZE, rbuf_index);



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



