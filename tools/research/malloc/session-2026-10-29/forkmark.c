typedef unsigned int u32; typedef unsigned long long u64;
#define SYS_open 5
#define SYS_write 4
#define SYS_close 6
#define SYS_fork 2
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d){register long r7 asm("r7")=n;register long r0 asm("r0")=a;register long r1 asm("r1")=b;register long r2 asm("r2")=c;register long r3 asm("r3")=d;asm volatile("svc #0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3):"memory");return r0;}
static void w(int fd,const char*s){int n=0;while(s[n])n++;sc(SYS_write,fd,(long)s,n,0);}
__attribute__((constructor)) static void ctor(void){long p=sc(SYS_fork,0,0,0,0);if(p==0){int fd=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/forkmark.log",0x241,0666,0);if(fd>=0){w(fd,"CHILD\n");sc(SYS_close,fd,0,0,0);}sc(SYS_exit,0,0,0,0);} }
