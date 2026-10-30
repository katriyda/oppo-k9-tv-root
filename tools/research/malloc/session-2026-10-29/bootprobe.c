#define SYS_open 5
#define SYS_read 3
#define SYS_write 4
#define SYS_close 6
#define SYS_exit 1
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static long lg;
static void put(const char*s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}
static void hx(unsigned long v){char b[20];int i;for(i=0;i<16;i++){int d=(v>>((15-i)*4))&15;b[i]=d<10?48+d:87+d;}b[16]=10;b[17]=0;put(b);}
int JNI_OnLoad(void*e,void*r){char buf[128];long fd;lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/bootprobe.log",0x241,0644,0,0,0);put("start\n");fd=sc(SYS_open,(long)"/dev/block/by-name/boot",0,0,0,0,0);put("boot fd=");hx(fd);if(fd>=0){long n=sc(SYS_read,fd,(long)buf,8,0,0,0);put("n=");hx(n);unsigned long w=0;int i;for(i=0;i<8&&i<n;i++)w|=(unsigned char)buf[i]<<(8*i);put("w=");hx(w);}put("end\n");return 0x10006;}
