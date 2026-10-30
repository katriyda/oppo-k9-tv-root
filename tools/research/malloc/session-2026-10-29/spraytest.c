#define SYS_socket 281
#define SYS_setsockopt 294
#define SYS_close 6
#define SYS_open 5
#define SYS_write 4
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static long lg;static void put(const char*s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}static void hx(unsigned long v){char b[12];int i;for(i=0;i<8;i++){int d=(v>>((7-i)*4))&15;b[i]=d<10?48+d:87+d;}b[8]=10;b[9]=0;put(b);}
int JNI_OnLoad(void*a,void*b){char opt[16];int i;long s,r;for(i=0;i<16;i++)opt[i]=(char)(i+1);lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/spraytest.log",0x241,0644,0,0,0);put("start\n");s=sc(SYS_socket,2,2,0,0,0,0);put("sock=");hx(s);if(s>=0){r=sc(SYS_setsockopt,s,1,25,(long)opt,16,0);put("set r=");hx(r);put("errno=");hx(-r);}put("end\n");return 0x10006;}
