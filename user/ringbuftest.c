#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "stdbool.h"


void test_find_1() {

  char **vm_addr[1];
  ringbuf("pranav", (void**) vm_addr);
  ringbuf("pranav", (void**) vm_addr);
  ringbuf("pranav", (void**) vm_addr);
  ringbuf("pranav", (void**) vm_addr);
}

void test_find_2() {

  char **vm_addr[1];
  for(int i=0;i<10;i++) {
    char s[5];
    s[0] = 'a'+i;
    s[1] = '\0';
    ringbuf(s, (void**) vm_addr); 
  }
}


int
main(int argc, char *argv[])
{
  //test_find_1();
  test_find_1();

  exit(0);
}
