#define SYS_open 5
#define SYS_close 6
#define SYS_write 4
#define SYS_getuid 199
#define SYS_setfsuid 138
#define SYS_setfsuid32 215
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static void w(int fd,const char*s){int n=0;while(s[n])n++;sc(SYS_write,fd,(long)s,n,0,0,0);}
static void dc(int fd,long v){char b[32];int i=31;b[31]=0;if(v<0){w(fd,"-");v=-v;}if(!v){w(fd,"0");return;}while(v&&i){b[--i]='0'+v%10;v/=10;}w(fd,b+i);}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){int lf=sc(SYS_open,(long)"/sdcard/setfsuid.log",0x241,0644,0,0,0);w(lf,"START uid=");dc(lf,sc(SYS_getuid,0,0,0,0,0,0));w(lf,"\n");long r=sc(SYS_setfsuid,12345,0,0,0,0,0);w(lf,"r=");dc(lf,r);w(lf," uid=");dc(lf,sc(SYS_getuid,0,0,0,0,0,0));w(lf,"\n");long r2=sc(SYS_setfsuid32,12345,0,0,0,0,0);w(lf,"r32=");dc(lf,r2);w(lf,"\n");sc(SYS_setfsuid,0,0,0,0,0,0);sc(SYS_close,lf,0,0,0,0,0);return 0x10006;}
