typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long long u64;
#define SYS_open 5
#define SYS_close 6
#define SYS_write 4
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){
 register long r7 __asm__("r7")=n; register long r0 __asm__("r0")=a;
 register long r1 __asm__("r1")=b; register long r2 __asm__("r2")=c;
 register long r3 __asm__("r3")=d; register long r4 __asm__("r4")=e;
 register long r5 __asm__("r5")=f;
 __asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");
 return r0;
}
static void w(int fd,const char*s){int n=0;while(s[n])n++;if(fd>=0)sc(SYS_write,fd,(long)s,n,0,0,0);}
static void hx(int fd,u64 v){char b[17];int i;for(i=15;i>=0;i--){int d=v&0xf;b[i]=d<10?'0'+d:'a'+d-10;v>>=4;}b[16]=0;w(fd,b);}
static void dc(int fd,long v){char b[32];int i=31;b[31]=0;if(v<0){w(fd,"-");v=-v;}if(!v){w(fd,"0");return;}while(v&&i){b[--i]='0'+v%10;v/=10;}w(fd,b+i);}
static void st(int fd,const u8*p,int n){for(int i=0;i<n;i++){u8 c=p[i];if(c>=32&&c<127)sc(SYS_write,fd,(long)&c,1,0,0,0);else w(fd,".");}}
struct info { u64 addr; u64 size; u64 interval; u8 miusel; u8 pad[7]; };
static int pfd=-1;
static int lf=-1;
static long mp(u64 bus,u64 size){
 struct info i;i.addr=bus-0x20000000ULL;i.size=size;i.interval=0;i.miusel=0;for(int k=0;k<7;k++)i.pad[k]=0;
 long r=sc(SYS_ioctl,pfd,0x80204d04,(long)&i,0,0,0);
 w(lf,"ioctl=");dc(lf,r);w(lf," addr=");hx(lf,i.addr);w(lf," size=");dc(lf,i.size);w(lf," ");
 if(r<0)return 0;
 long p=sc(SYS_mmap2,0,size,3,1,pfd,0);
 w(lf,"mmap=");dc(lf,p);w(lf,"\n");
 if((unsigned long)p>=0xfffff000UL)return 0;
 return p;
}
static void dump_task(const char*tag,u64 bus,u64 baseoff){
 u64 page=bus&~0xfffULL; u32 off=(u32)(bus&0xfff);
 long p=mp(page,0x2000);
 w(lf,tag);w(lf," bus=");hx(lf,bus);w(lf," page=");hx(lf,page);w(lf,"\n");
 if(!p)return;
 u8*b=(u8*)p;
 w(lf," comm@5e8=");st(9,b+off+0x5e8,16);w(lf,"\n");
 w(lf," comm@2c8=");st(9,b+off+0x2c8,16);w(lf,"\n");
 w(lf," q320=");hx(lf,*(u64*)(b+off+0x320));w(lf," q328=");hx(lf,*(u64*)(b+off+0x328));w(lf,"\n");
 w(lf," q5d8=");hx(lf,*(u64*)(b+off+0x5d8));w(lf," q5e0=");hx(lf,*(u64*)(b+off+0x5e0));w(lf,"\n");
 w(lf," q890=");hx(lf,*(u64*)(b+off+0x890));w(lf," q898=");hx(lf,*(u64*)(b+off+0x898));w(lf,"\n");
 sc(SYS_munmap,p,0x2000,0,0,0,0);
}
static void dump_raw(const char*tag,u64 bus,int len){
 u64 page=bus&~0xfffULL; u32 off=(u32)(bus&0xfff);
 long p=mp(page,0x1000);
 w(lf,tag);w(lf," bus=");hx(lf,bus);w(lf,"\n");if(!p)return;
 u8*b=(u8*)p+off;
 for(int row=0;row<len;row+=16){w(lf," ");hx(lf,bus+row);w(lf," ");for(int c=0;c<16;c++){u8 v=b[row+c];char h[3];h[0]="0123456789abcdef"[v>>4];h[1]="0123456789abcdef"[v&15];h[2]=0;w(lf,h);w(lf," ");}w(lf," |");st(9,b+row,16);w(lf,"|\n");}
 sc(SYS_munmap,p,0x1000,0,0,0,0);
}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){
 lf=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/probe_mem.log",0x241,0644,0,0,0);
 if(lf<0)return 0x10006;
 w(lf,"START\n");
 pfd=sc(SYS_open,(long)"/dev/malloc",2,0,0,0,0);
 w(lf,"pfd=");dc(lf,pfd);w(lf,"\n");if(pfd<0)return 0x10006;
 dump_raw("INITRAW",0x2121a880,0x40);
 dump_task("INIT",0x2121a880,0);
 dump_task("KTH",0x8fed4000,0);
 dump_task("KTH_NODE",0x8fed4320,0);
 dump_task("ALT",0xafed4000,0);
 w(lf,"DONE\n");
 sc(SYS_close,lf,0,0,0,0,0);
 return 0x10006;
}
