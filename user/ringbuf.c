#include "ringbuf.h"
#include "user.h"

struct ringbook
{
  uint64 read;
  uint64 write;
};

struct user_ring_buf {
  char *name;
  uint64* addr;
  int exists;
}user_ring_bufs[MAX_RINGBUFS];

int
strncmp(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

// It writes v into *p.
void store(uint64 *p, uint64 v) {
  __atomic_store_8(p, v, __ATOMIC_SEQ_CST);
}

// It returns the contents of *p.
int load(uint64 *p) {
  return __atomic_load_8(p, __ATOMIC_SEQ_CST);
}

// find the position of ring buffer by name
int find(char* name) {
    int index=-1;
    for(int i=0;i<MAX_RINGBUFS;i++) {
        if(strncmp(name, user_ring_bufs[i].name, strlen(name)) == 0) {
            if(user_ring_bufs[i].exists==1)
                index=i;
        }
    }
    return index;
}

// invokes ringbuf(name, addr, 0)
// return -1 on fail
// otherwise return ring buffer descriptor
int create_ring_buffer(char *name) {
    uint64 vm_addr;
    int res = ringbuf(name,&vm_addr,0);
    if(res==-1) {
        return -1;
    }
    uint64* start = (uint64*) vm_addr;
    struct ringbook book;
    book.read=0;
    book.write=0;
    // *(start) = book.
    *(start+32*512) = book.read;
    *(start+32*512+8) = book.write;
    int empty_slot=-1;
    for(int i=0;i<MAX_RINGBUFS;i++){
        if(user_ring_bufs[i].exists==0){
            user_ring_bufs[i].name=name;
            user_ring_bufs[i].addr=start;
            user_ring_bufs[i].exists=1;
            empty_slot=i;
            break;
        }
    }
    if(empty_slot==-1){
      printf("Ring buffers are full.\n");
      return -1;
    } else {
      return empty_slot;
    }
}

// invokes create_ringbuf(name, addr, 1)
// return -1 on fail
int delete_ring_buffer(char *name){
    int index=find(name);
    if(index==-1){
      printf("Ring buffer %s not found.\n",name);
      return -1;
    }
    int res = ringbuf(name,user_ring_bufs[index].addr,1);
    if(res==-1) {
      return -1;
    }
    user_ring_bufs[index].exists=0;
    return 0;
}

void ringbuf_start_read(int index, uint64 **addr, int *bytes) {
    if(index<0||index>=MAX_RINGBUFS){
      printf("Ring buffer not found.\n");
      return;
    }
    uint64* start=user_ring_bufs[index].addr;
    uint64 read=load(start+32*512);   //  load 'read' from 'book' 
    uint64 write=load(start+32*512+8);    // load 'write' from 'book'
    *addr=start+read;
    *bytes=(write-read)%(RINGBUF_SIZE*PG_SIZE);
}

void ringbuf_finish_read(int index, int bytes) {
    if(index<0||index>=MAX_RINGBUFS){
      printf("Ring buffer not found.\n");
      return;
    }
    uint64* start=user_ring_bufs[index].addr;
    uint64 read=load(start+32*512);   //  load 'read' from 'book' 
    store(start+32*512,(read+bytes)%(RINGBUF_SIZE*PG_SIZE));
}

void ringbuf_start_write(int index, uint64 **addr, int *bytes) {
    if(index<0||index>=MAX_RINGBUFS){
      printf("Ring buffer not found.\n");
      return;
    }
    uint64* start=user_ring_bufs[index].addr;
    uint64 read=load(start+32*512);   //  load 'read' from 'book' 
    uint64 write=load(start+32*512+8);    // load 'write' from 'book'
    *addr=start+write;
    *bytes=(read-(write+1))%(RINGBUF_SIZE*PG_SIZE)+1;
}

void ringbuf_finish_write(int index, int bytes) {
    if(index<0||index>=MAX_RINGBUFS){
      printf("Ring buffer not found.\n");
      return;
    }
    uint64* start=user_ring_bufs[index].addr;
    uint64 write=load(start+32*512+8);    // load 'write' from 'book'
    store(start+32*512+8,(write+bytes)%(RINGBUF_SIZE*PG_SIZE));
}
