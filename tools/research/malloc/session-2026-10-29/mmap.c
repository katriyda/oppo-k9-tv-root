#define SYS_open 5
#define SYS_write 4
#define SYS_close 6
#define SYS_mmap2 192
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static void lg(const char*s){long f=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/mmaplog",0x241,0644,0,0,0);if(f>=0){int n=0;while(s[n])n++;sc(SYS_write,f,(long)s,n,0,0,0);sc(SYS_close,f,0,0,0,0,0);}}
static void hx(unsigned long v){char b[20];int i;for(i=0;i<16;i++){int d=(v>>((15-i)*4))&15;b[i]=d<10?48+d:87+d;}b[16]=10;b[17]=0;lg(b);}
__attribute__((constructor)) static void init(void){lg("mmap start\n");long fd=sc(SYS_open,(long)"/dev/miomap",2,0,0,0,0);lg("fd ");hx(fd);if(fd>=0){long p=sc(SYS_mmap2,0,0x1000,3,1,fd,0x20280);lg("ptr ");hx(p);if(p>=0 && p!=-1){unsigned int x=*(volatile unsigned int*)p;lg("word ");hx(x);}}lg("mmap end\n");}
