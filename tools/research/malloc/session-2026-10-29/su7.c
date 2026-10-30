#define SYS_setuid 213
#define SYS_setgid 214
#define SYS_execve 11
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
void _start(void){sc(SYS_setgid,0,0,0,0,0,0);sc(SYS_setuid,0,0,0,0,0,0);char*av[2];av[0]=(char*)"/system/bin/id";av[1]=0;char*envp[1];envp[0]=0;sc(SYS_execve,(long)av[0],(long)av,(long)envp,0,0,0);sc(SYS_exit,1,0,0,0,0,0);}
