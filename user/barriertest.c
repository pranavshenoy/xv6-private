#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"


void multiple_fork(int count, int fd) {

  for(int i=1;i<=count;i++) {
    int pid = fork();
    if(pid == 0) {
      int res = read(fd, &count, 4);
      if(res == -1) {
        printf("read failed\n");
      } else {
        printf("Total processes waiting: %d\n", count);
      }
    } else {
      break;
    }
  }
}


int main() {

  int fd = open("barrier0", O_RDWR); 
  multiple_fork(10, fd);
  sleep(10);
  int dummy = 5;
  write(fd, &dummy, 4);
  printf("done writing \n");
  return 0;
}
