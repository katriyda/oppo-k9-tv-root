#define SYS_open 5
#define SYS_write 4
#define SYS_close 6
static long sc(long n,long a,long b,long c){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2):"memory");return r0;}
__attribute__((constructor)) static void init(void){const char *p="/data/data/com.cvte.factory.service/files/hello";long f=sc(SYS_open,(long)p,0x241,0644);if(f<0){p="/data/local/tmp/hello.out";f=sc(SYS_open,(long)p,0x241,0666);}if(f>=0){sc(SYS_write,f,(long)"APPEXEC-SO\n",12);sc(SYS_close,f,0,0);} }
