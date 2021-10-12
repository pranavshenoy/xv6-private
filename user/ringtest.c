#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h" 
#include "ringbuf.h"

#define SIZE 128        // size of each int array, containing 512 bytes
#define LOOP 20480      // the number of loops for transmitting 10 MB
//#define DATA_SIZE 10*10
#define DATA_SIZE 10*1024*1024/8
#define NAME "test"


//returns the bytes written to fd
int write_buf(int fd, int bytes) {
  
  return 0;
}

int read_buf(int fd, int bytes) {
  
  return 0;
}


int main(void)
{
    /*
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
    }*/

    int pid=fork();
    if(pid>0)
    {
        printf("parent: child pid=%d\n",pid);
        int start=uptime();

        int index=create_ring_buffer(NAME);
        if(index==-1){
            printf("create error\n");
            exit(1);
        }
        uint64 rbytes=0;   // count number of read bytes
        uint64* Raddr;
        int Rbytes;
        
        while(rbytes<DATA_SIZE)
        {
            ringbuf_start_read(index,&Raddr,&Rbytes);
            if(Rbytes>0){
                uint64 n=Rbytes/sizeof(uint64);
                int cnt=0;
                for(int i=0;i<n;i++){    
                    uint64 res=*(Raddr+i);
                    if(res!=rbytes) printf("error: read %d should be %d\n",res,rbytes);
                    rbytes++;
                    cnt++;
                    printf("addr: %p; bytes can read: %d; reading data: %d\n",Raddr+i,Rbytes-i*sizeof(uint64),res);
                    if(rbytes>=DATA_SIZE)break;
                }
                ringbuf_finish_read(index,cnt*sizeof(uint64));
            }
        }

        pid=wait(0);
        int end=uptime();
        printf("parent: child %d is done, %d bytes have been read, time cost: %d ticks\n",pid,rbytes*sizeof(uint64),(end-start));
        /*index=delete_ring_buffer(NAME);
        if(index==-1){
            printf("delete error\n");
            exit(1);
        }*/
        exit(0);
    }
    else if(pid==0)
    {
        printf("child: start writing\n");
        
        int index=create_ring_buffer(NAME);
        if(index==-1){
            printf("create error\n");
            exit(1);
        }
        uint64 wbytes=0;   // count number of written bytes
        uint64* Waddr;
        int Wbytes;
        
        while(wbytes<DATA_SIZE)
        {
            ringbuf_start_write(index,&Waddr,&Wbytes);
            if(Wbytes>0){
                uint64 n=Wbytes/sizeof(uint64);
                int cnt=0;
                for(int j=0;j<n;j++){
                    *(Waddr+j)=wbytes;
                    printf("addr: %p; bytes can write: %d; writing data: %d\n",Waddr+j,Wbytes-j*sizeof(uint64),wbytes);
                    wbytes++;
                    cnt++;
                    if(wbytes>=DATA_SIZE)break;
                }
                ringbuf_finish_write(index,cnt*sizeof(uint64));
            }
        }
        printf("child: finish writing, %d bytes have been written\n",wbytes*sizeof(uint64));
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
