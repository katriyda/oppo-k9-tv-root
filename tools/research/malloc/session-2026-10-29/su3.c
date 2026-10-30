typedef unsigned int u32; typedef unsigned long long u64;
#define SYS_setresgid 165
#define SYS_setresuid 164
#define SYS_execve 11
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){
 register long r7 __asm__("r7")=n; register long r0 __asm__("r0")=a;
 register long r1 __asm__("r1")=b; register long r2 __asm__("r2")=c;
 register long r3 __asm__("r3")=d; register long r4 __asm__("r4")=e;
 register long r5 __asm__("r5")=f;
 __asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");
 return r0;
}
void start_c(long *sp);
__attribute__((naked)) void _start(void){__asm__ volatile("mov r0, sp\n b start_c\n");}
void start_c(long *sp){
 long argc=sp[0]; char**argv=(char**)(sp+1); char**envp=argv+argc+1;
 sc(SYS_setresgid,0,0,0,0,0,0); sc(SYS_setresuid,0,0,0,0,0,0);
 char *av[5];
 if(argc>=3 && argv[1][0]=='-' && argv[1][1]=='c' && argv[1][2]==0){
  av[0]=(char*)"/system/bin/sh"; av[1]=(char*)"-p"; av[2]=(char*)"-c"; av[3]=argv[2]; av[4]=0;
 } else { av[0]=(char*)"/system/bin/sh"; av[1]=(char*)"-p"; av[2]=0; }
 sc(SYS_execve,(long)av[0],(long)av,(long)envp,0,0,0);
 sc(SYS_exit,1,0,0,0,0,0);
}
