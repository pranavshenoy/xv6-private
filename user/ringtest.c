#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h" 
#include "ringbuf.h"

#define SIZE 128        // size of each int array, containing 512 bytes
#define LOOP 20480      // the number of loops for transmitting 10 MB
#define DATA_SIZE 10
#define NAME "test"

int main(void)
{
    int index=create_ring_buffer(NAME);
    if(index==-1){
        printf("create error\n");
        exit(1);
    }
    uint64* addr;
    int bytes;
    for(int i=0;i<DATA_SIZE;i++){
        ringbuf_start_write(index,&addr,&bytes);
        printf("addr: %p; bytes can write: %d; ",addr,bytes);
        *addr=i;
        printf("writing data: %d\n",i);
        ringbuf_finish_write(index,1);
    }
    for(int i=0;i<DATA_SIZE;i++){
        ringbuf_start_read(index,&addr,&bytes);
        printf("addr: %p; bytes can read: %d; ",addr,bytes);
        uint64 res=*addr;
        printf("reading data: %d\n",res);
        ringbuf_finish_read(index,1);
    }

    int pid=fork();
    if(pid>0)
    {
        printf("parent: child pid=%d\n",pid);
        int start=uptime();
        uint64 rbytes=0;   // count number of read bytes
        uint64* Raddr;
        int Rbytes;
        
        while(rbytes<DATA_SIZE)
        {
            ringbuf_start_read(index,&Raddr,&Rbytes);
            if(Rbytes>0){
                uint64 res=*Raddr;
                if(res!=rbytes) printf("error: read %d should be %d\n",res,rbytes);
                rbytes++;
                ringbuf_finish_read(index,1);
            }
        }

        pid=wait(0);
        int end=uptime();
        printf("parent: child %d is done, %d bytes have been read, time cost: %d ticks\n",pid,rbytes,(end-start));
        index=delete_ring_buffer(NAME);
        if(index==-1){
            printf("delete error\n");
            exit(1);
        }
        exit(0);
    }
    else if(pid==0)
    {
        printf("child: start writing\n");
        uint64 wbytes=0;   // count number of written bytes
        uint64* Waddr;
        int Wbytes;
        
        while(wbytes<DATA_SIZE)
        {
            ringbuf_start_write(index,&Waddr,&Wbytes);
            if(Wbytes>0){
                *Waddr=wbytes;
                wbytes++;
                ringbuf_finish_write(index,1);
            }
        }
        printf("child: finish writing, %d bytes have been written\n",1);
        exit(0);
    }
    else
    {
        printf("fork error\n");
        exit(1);
    }
    
    exit(0);
    return 0;
}