typedef unsigned int u32;
#define SYS_open 5
#define SYS_write 4
#define SYS_close 6
#define SYS_getuid 199
#define SYS_geteuid 201
#define SYS_setresuid32 208
#define SYS_setresgid32 209
#define SYS_execve 11
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static int lf=-1;static void w(const char*s){int n=0;while(s[n])n++;if(lf>=0)sc(SYS_write,lf,(long)s,n,0,0,0);}static void dc(long v){char b[32];int i=31;b[31]=0;if(v<0){w("-");v=-v;}if(!v){w("0");return;}while(v&&i){b[--i]='0'+v%10;v/=10;}w(b+i);}
void _start(void){lf=sc(SYS_open,(long)"/sdcard/su6.txt",0x241,0644,0,0,0);w("before u=");dc(sc(SYS_getuid,0,0,0,0,0,0));w(" e=");dc(sc(SYS_geteuid,0,0,0,0,0,0));w("\n");long a=sc(SYS_setresgid32,0,0,0,0,0,0);long b=sc(SYS_setresuid32,0,0,0,0,0,0);w("g=");dc(a);w(" u=");dc(b);w(" after ");dc(sc(SYS_getuid,0,0,0,0,0,0));w("/");dc(sc(SYS_geteuid,0,0,0,0,0,0));w("\n");char*av[2];av[0]=(char*)"/system/bin/id";av[1]=0;char*envp[1];envp[0]=0;sc(SYS_execve,(long)av[0],(long)av,(long)envp,0,0,0);sc(SYS_exit,1,0,0,0,0,0);}
