#include "user.h"
#include "../kernel/types.h"

#define RINGBUF_SIZE 16
#define MAX_RINGBUFS 10
#define PG_SIZE 4096

int create_ring_buffer(char *name);

int delete_ring_buffer(char *name);

void store(int *p, int v);

int load(int *p);

void ringbuf_start_read(char *name, uint64 **addr, int *bytes);
void ringbuf_finish_read(char *name, int bytes);

void ringbuf_start_write(char *name, uint64 **addr, int *bytes);
void ringbuf_finish_write(char *name, int bytes);
