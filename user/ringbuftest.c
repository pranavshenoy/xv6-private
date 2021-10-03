#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "stdbool.h"

void test_find_0() {

  uint64 vm_addr;
  ringbuf("pranav", &vm_addr, 0);
  printf("Address received to caller %p\n", vm_addr);
  uint64* start = (uint64*) vm_addr;
  for(int i=0; i<16;i++) {
    *(start+i*4096) = i*10;
  }

  for(int i=0;i<32;i++) {
    printf("i: %d address: %p value: %d\n", i, start+i*4096, *(start+i*4096));
  }
}

void test_find_1() {

  uint64 vm_addr;
  ringbuf("pranav", &vm_addr, 0);
  ringbuf("pranav", &vm_addr, 0);
  ringbuf("pranav", &vm_addr, 0);
  ringbuf("pranav", &vm_addr, 0);
}

void test_find_2() {

  uint64 vm_addr;
  for(int i=0;i<14;i++) {
    char s[5];
    s[0] = 'a'+i;
    s[1] = '\0';
    ringbuf(s, &vm_addr, 0); 
  }
}

int
main(int argc, char *argv[])
{
  //test_find_1();
  test_find_0();

  exit(0);
}
