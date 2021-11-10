#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"


int main() {

  int fd = open("rdcycle", O_RDWR); //giving both permission to test write as well
  int n = 5;
  while(n--) {
    unsigned long cycle;
    int res = read(fd, &cycle, 8);
    if(res == -1) {
      printf("read failed\n");
    } else {
      printf("cycles: %ld", cycle);
    }
  }
  return 0;
}
