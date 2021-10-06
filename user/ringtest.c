#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "user.h" 
#include "ringbuf.h"

#define SIZE 128        // size of each int array, containing 512 bytes
#define LOOP 20480      // the number of loops for transmitting 10 MB
#define NAME "test"

int main(void)
{
    int index=create_ring_buffer(NAME);
    if(index==-1){
        printf("create error\n");
        exit(1);
    }
    
    int pid=fork();
    if(pid>0)
    {
        printf("parent: child pid=%d\n",pid);
        int start=uptime();
        int rbytes=0;   // count number of read bytes
        for(int r=0;r<LOOP;r++)
        {
            uint64 rbytes=0;
            uint64 addr;
            int bytes;
            for(int i=0;i<SIZE;i++){
                ringbuf_start_read(index,addr,bytes);
                uint64 res=load(addr+i);
                if(res!=rbytes)
                {
                    printf("error\n");
                    exit(1);
                }
                rbytes++;
                ringbuf_finish_read(index,1);
            }
        }
        pid=wait(0);
        int end=uptime();
        printf("parent: child %d is done, %d bytes have been read, time cost: %d ticks\n",pid,rbytes,(end-start));
        exit(0);
    }
    else if(pid==0)
    {
        printf("child: start writing\n");
        uint64 wbytes=0;   // count number of written bytes
        for(int w=0;w<LOOP;w++)
        {
            uint64 addr;
            int bytes;
            for(int i=0;i<SIZE;i++){
                ringbuf_start_write(index,addr,bytes);
                store(addr+i,wbytes);
                wbytes++;
                ringbuf_finish_write(index,1);
            }
        }
        printf("child: finish writing, %d bytes have been written\n",wbytes);
        exit(0);
    }
    else
    {
        printf("fork error\n");
        exit(1);
    }
    return 0;
}