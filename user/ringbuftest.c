#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "stdbool.h"



int
main(int argc, char *argv[])
{
  char **vm_addr[1];

//  printf(" Address before ringbuf %p\n",vm_addr);
  ringbuf("pranav", (void**) vm_addr);
//  printf("address passed: %p and - address in the user space %p\n", vm_addr,  *vm_addr);

  exit(0);
}
