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
  //TODO: validate name
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


int deallocate_pm(int pages, void* phy_addr) {

  for(int i=0;i<pages;i++) {
    kfree(phy_addr+i);
  }
  return 0;
}

int allocate_pm(int pages, void** phy_addr) {
  for(int i=0;i<pages;i++) {
    void* ptr = kalloc();
    *(phy_addr+i) = ptr;
    printf("i: %d Phy mem address in pointer %p\n", i, ptr);
    if(*(phy_addr+i) == 0) {
      deallocate_pm(i, phy_addr);
      return -1;
    }
  }
  return 0;
}

int update_rbuffer(char* name, int rbuf_index) {

  rb_arr[rbuf_index].refcount += 1;
  strncpy(rb_arr[rbuf_index].name, name, strlen(name));
  if(allocate_pm(R_BUF_SIZE, rb_arr[rbuf_index].pa) != 0) {
    return -1;
  }
  return 0;
}

//-- Virtual Address mapping
void display_vm(pagetable_t pagetable, uint64 virt_addr, int pages) {

  printf("start of display_vm %p\n", virt_addr);
  /*
  for(int i=0;i<pages/2;i++) {
    uint64* tmp = (uint64*) virt_addr;
    printf("tmp %p\n", tmp);
    *tmp = (uint64) 344;
  }
  */
  printf("stored values\n");
  for(int i=0;i<pages;i++) {
    //uint64 *tmp = (uint64*) (virt_addr+i*4096);
    uint64 pa = walkaddr(pagetable, virt_addr+i*4096);
    printf("i: %d vm:  %p, pm: %p  \n", i+1, virt_addr+i*4096, pa);
    //printf("i: %d vm:  %p, pm: %p %d \n", i+1, virt_addr+i*4096, pa, *tmp);
  }
  printf("end of display_vm\n");
}

void unmap_va(uint64 virt_addr, int pages) {

  if(pages < 0) {
    return;
  }
  struct proc *p = myproc();
  uvmunmap(p->pagetable, virt_addr, pages, 0);
}

//check if the number of pages in va is free starting from va. return 0 if all pages are free, else return -1.
int verify_va(pagetable_t pagetable, uint64 va, int pages) {
  for(int i=0;i<pages;i++) {
    if(walkaddr(pagetable, va-i*PG_SZ) != 0) {
      return -1;
    }
  }
  return 0;
}

int get_free_va(pagetable_t pagetable, uint64* virt_addr, int pages) {
  
  uint64 start_addr = MAXVA - 36*PG_SZ; // 1 trampoline, 1 trapframe, leaving 1 page for safety

  int retries = 10;
  uint64 gap = 50*PG_SZ;
  int i	= 0;
  while(i<retries) {
    uint64 addr = start_addr-(i*gap);
    if(verify_va(pagetable, addr, pages) == 0) {
      *virt_addr = addr - (pages*PG_SZ);
      return 0;
    }
    i++;
  }
  return -1;

}

int map_va(pagetable_t pagetable, void** phy_buffer, uint64* virt_addr, int phy_pages) {
/*
  int ring_buf_size = phy_pages - 1;
  for(int i=0;i<2*ring_buf_size;i++) {
    uint64 va = *virt_addr + i*PG_SZ;
    uint64 pa = (uint64) *(phy_buffer + i%ring_buf_size);
    printf("i: %d virt address %p, phy address: %p\n", i, va, pa);
    if(mappages(pagetable, va, 1, pa, PTE_W|PTE_R|PTE_X|PTE_U) != 0) {
      unmap_va(*virt_addr, i);
      return -1;
    }
  }
  uint64 va = *virt_addr + 2*ring_buf_size*PG_SZ;
  uint64 pa = (uint64) *(phy_buffer + 2*ring_buf_size);
  printf("virt address %p, phy address: %p\n", va, pa);
  if(mappages(pagetable, va, 1, pa, PTE_W|PTE_R|PTE_X|PTE_U) != 0) {
    unmap_va(*virt_addr, 2*ring_buf_size);
    return -1;
  }
  printf("mapping successful\n");
  return 0;
*/

  int ring_buf_size = phy_pages - 1;  
  for(int i=0;i<ring_buf_size*2;i++) {
    if(mappages(pagetable, (uint64)(*virt_addr+i*PG_SZ), 1, (uint64)*(phy_buffer + i%ring_buf_size), PTE_W|PTE_R|PTE_X|PTE_U) != 0) {
      unmap_va(*virt_addr, i);
      return -1;
    }
  }
  if(mappages(pagetable, (uint64)(*virt_addr+ 2*ring_buf_size*PG_SZ), 1, (uint64)*(phy_buffer + ring_buf_size), PTE_W|PTE_R|PTE_X|PTE_U) != 0) {
    unmap_va(*virt_addr, ring_buf_size*2);
    return -1;
  }
  return 0;
}

int create_va(pagetable_t pagetable, uint64* virt_addr, int rb_index, int phy_pages) {

  uint64 ptr;
  if(get_free_va(pagetable, &ptr, phy_pages*2-1) != 0) {
    return -1;
  }
  printf("virtual address: %p\n", ptr);
  if(map_va(pagetable, rb_arr[rb_index].pa, &ptr, phy_pages) != 0) {
    return -1;
  }
  printf("virtual address after mapping: %p\n", ptr);
  display_vm(pagetable, ptr, phy_pages*2-1);
  unmap_va(ptr, 2*phy_pages-1);
  *virt_addr = ptr;
  return 0;
}


//TESTING
void display_pm(int pages, int rbuf_index) {
  for(int i=0;i<pages;i++) {
    printf("Physical memory allocated %p \n", (uint64) *(rb_arr[rbuf_index].pa+i));
  }
}


//TODO: rename to a generic one
uint64
create_ringbuf(char* name, uint64* vm_addr) {

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
    if(update_rbuffer(name, rbuf_index) != 0) {
      printf("unable to allocate physical memory\n");
      return -1;
    }
    printf("Received a free index: %d for name: %s\n", rbuf_index, name);
  }
  struct proc *p = myproc();
  uint64 va;
  if(create_va(p->pagetable, &va, rbuf_index, R_BUF_SIZE) != 0) {
    printf("unable to map virtual memory address\n");
    //TODO: cleanup
    return -1;
  }
  vm_addr = (uint64*)va;
  printf("ending mapping allocation\n");
  return 0;
}



