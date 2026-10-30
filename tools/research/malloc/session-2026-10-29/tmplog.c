#define SYS_open 5
#define SYS_write 4
#define SYS_close 6
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){
 const char*p[4]={"/data/local/tmp/tmplog","/sdcard/tmplog","/storage/emulated/0/tmplog","/data/data/com.cvte.factory.service/tmplog"};
 for(int i=0;i<4;i++){long fd=sc(SYS_open,(long)p[i],0x241,0666,0,0,0);if(fd>=0){sc(SYS_write,fd,(long)"OK\n",3,0,0,0);sc(SYS_close,fd,0,0,0,0,0);}}
 return 0x10006;
}
