#define SYS_open 5
#define SYS_close 6
#define SYS_write 4
#define SYS_read 3
#define SYS_mkdir 39
#define SYS_mount 21
#define SYS_chmod 15
#define SYS_setns 375
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static int lf=-1;
static void w(const char*s){int n=0;while(s[n])n++;if(lf>=0)sc(SYS_write,lf,(long)s,n,0,0,0);}
static void dc(long v){char b[32];int i=31;b[31]=0;if(v<0){w("-");v=-v;}if(!v){w("0");return;}while(v&&i){b[--i]='0'+v%10;v/=10;}w(b+i);}
static int cpy(const char*src,const char*dst){int in=sc(SYS_open,(long)src,0,0,0,0,0);if(in<0)return -1;int out=sc(SYS_open,(long)dst,0x241,0755,0,0,0);if(out<0){sc(SYS_close,in,0,0,0,0,0);return -2;}char buf[4096];long n;while((n=sc(SYS_read,in,(long)buf,4096,0,0,0))>0)sc(SYS_write,out,(long)buf,n,0,0,0);sc(SYS_close,in,0,0,0,0,0);sc(SYS_close,out,0,0,0,0,0);return 0;}
void _start(void){
 lf=sc(SYS_open,(long)"/sdcard/mounter.log",0x241,0644,0,0,0);w("START\n");
 int fd=sc(SYS_open,(long)"/proc/1/ns/mnt",0,0,0,0,0);w("nsopen=");dc(fd);w("\n");
 if(fd>=0){long r=sc(SYS_setns,fd,0x20000,0,0,0,0);w("setns=");dc(r);w("\n");sc(SYS_close,fd,0,0,0,0,0);}
 sc(SYS_mkdir,(long)"/data/local/tmp/su_mnt",0777,0,0,0,0);
 long r=sc(SYS_mount,(long)"tmpfs",(long)"/data/local/tmp/su_mnt",(long)"tmpfs",0,(long)"mode=0777,suid",0);
 w("mount=");dc(r);w("\n");
 long c=cpy("/data/local/tmp/su","/data/local/tmp/su_mnt/su");w("copy=");dc(c);w("\n");
 long ch=sc(SYS_chmod,(long)"/data/local/tmp/su_mnt/su",04755,0,0,0,0);w("chmod=");dc(ch);w("\n");
 w("DONE\n");sc(SYS_close,lf,0,0,0,0,0);sc(SYS_exit,0,0,0,0,0,0);
}
