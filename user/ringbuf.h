
#include "../kernel/types.h"

#define RINGBUF_SIZE 16
#define MAX_RINGBUFS 10
#define PG_SIZE 4096

int create_ring_buffer(char *name);

int delete_ring_buffer(char *name);

void store(uint64 *p, uint64 v);

int load(uint64 *p);

void ringbuf_start_read(int index, uint64 **addr, int *bytes);
void ringbuf_finish_read(int index, int bytes);

void ringbuf_start_write(int index, uint64 **addr, int *bytes);
void ringbuf_finish_write(int index, int bytes);
