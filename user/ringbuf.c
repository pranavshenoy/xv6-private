#include "ringbuf.h"

struct user_ring_buf {
  void *buf;
  void *book;
  int exists;
};

void store(int *p, int v) {
  __atomic_store_8(p, v, __ATOMIC_SEQ_CST);
}

int load(int *p) {
  return __atomic_load_8(p, __ATOMIC_SEQ_CST);
}

//- invokes sys_ringbuf(name, 0)
//- return -1 on fail
//- otherwise return ring buffer descriptor
int create_ring_buffer(char *name) {

}

//- invokes sys_ringbuf(name, 1)
//- return -1 on fail
int delete_ring_buffer(int ring_buffer_descriptor){

}

void ringbuf_start_read(int ring_desc, char **addr, int *bytes) {

}
void ringbuf_finish_read(int ring_desc, int bytes);

void ringbuf_start_write(int ring_desc, char **addr, int *bytes);
void ringbuf_finish_write(int ring_desc, int bytes);
