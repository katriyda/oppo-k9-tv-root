#define SYS_open 5
#define SYS_write 4
#define SYS_close 6
#define SYS_prctl 172
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
void _start(void){int lf=sc(SYS_open,(long)"/sdcard/nnp.txt",0x241,0644,0,0,0);long v=sc(SYS_prctl,39,0,0,0,0,0);char b[4];b[0]='0'+(v&0xf);b[1]='\n';b[2]=0;sc(SYS_write,lf,(long)b,2,0,0,0);sc(SYS_close,lf,0,0,0,0,0);sc(SYS_exit,0,0,0,0,0,0);}
