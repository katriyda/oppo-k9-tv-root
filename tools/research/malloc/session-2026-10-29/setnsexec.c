#define SYS_open 5
#define SYS_close 6
#define SYS_setns 375
#define SYS_execve 11
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
void _start(void){int fd=sc(SYS_open,(long)"/proc/1/ns/mnt",0,0,0,0,0);if(fd>=0){sc(SYS_setns,fd,0x20000,0,0,0,0);sc(SYS_close,fd,0,0,0,0,0);}char*argv[4];argv[0]=(char*)"/system/bin/sh";argv[1]=(char*)"-c";argv[2]=(char*)"/data/local/tmp/install.sh";argv[3]=0;char*envp[2];envp[0]=(char*)"PATH=/sbin:/system/sbin:/system/bin:/system/xbin";envp[1]=0;sc(SYS_execve,(long)argv[0],(long)argv,(long)envp,0,0,0);sc(SYS_exit,1,0,0,0,0,0);}
