
#include "types.h"

#define R_BUF_SIZE 17
#define R_BUF_NAME_SIZE 16
#define MAX_R_BUFS 10

extern uint64 create_ringbuf(char* name, uint64*  vm_addr);
