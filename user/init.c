// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = { "sh", 0 };
/*
const char* get_barrier_name(int barrier) {

  switch(barrier) {
  case 0: return "barrier0";
  case 1: return "barrier1";
  case 2: return "barrier2";
  }
  return "";
}
*/
void init_barriers() {

  mknod("barrier0", DEV_BARRIER, 0);
  mknod("barrier1", DEV_BARRIER, 1);
  mknod("barrier2", DEV_BARRIER, 2);
  mknod("barrier3", DEV_BARRIER, 3);
  mknod("barrier4", DEV_BARRIER, 4);
  mknod("barrier5", DEV_BARRIER, 5);
  mknod("barrier6", DEV_BARRIER, 6);
  mknod("barrier7", DEV_BARRIER, 7);
  mknod("barrier8", DEV_BARRIER, 8);
  mknod("barrier9", DEV_BARRIER, 9);
}

int
main(void)
{
  int pid, wpid;
  if(open("console", O_RDWR) < 0){
    mknod("console", CONSOLE, 0);
    open("console", O_RDWR);
  }

  mknod("rdcycle", DEV_CYCLE, 0);
  init_barriers();
  
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf("init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf("init: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      exec("sh", argv);
      printf("init: exec sh failed\n");
      exit(1);
    }

    for(;;){
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *) 0);
      if(wpid == pid){
        // the shell exited; restart it.
        break;
      } else if(wpid < 0){
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}
