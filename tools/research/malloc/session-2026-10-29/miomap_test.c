#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
static sigjmp_buf jb;
static void h(int s){siglongjmp(jb,s);} 
int main(int argc,char**argv){
 unsigned long off = argc>1?strtoul(argv[1],0,0):0;
 int fd=open("/dev/miomap",O_RDWR); if(fd<0){printf("open %d %s\n",errno,strerror(errno));return 1;}
 printf("fd=%d off=%lx\n",fd,off); fflush(stdout);
 signal(SIGBUS,h); signal(SIGSEGV,h);
 void *p=mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_SHARED,fd,off);
 printf("mmap=%p errno=%d\n",p,errno); fflush(stdout);
 if(p!=MAP_FAILED && sigsetjmp(jb,1)==0){ printf("read: "); for(int i=0;i<16;i++) printf("%02x",((unsigned char*)p)[i]); printf("\n"); fflush(stdout);} else if(p!=MAP_FAILED) printf("fault\n");
 sleep(1); return 0;
}
