#define SYS_open 5
#define SYS_write 4
#define SYS_close 6
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static long lg;static void put(const char*s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}static void hx(unsigned long v){char b[12];int i;for(i=0;i<8;i++){int d=(v>>((7-i)*4))&15;b[i]=d<10?48+d:87+d;}b[8]=10;b[9]=0;put(b);}
extern int MI_SYS_Init(void);
extern int MI_SYS_DeInit(void);
extern int MI_SYS_GetMmapLayout(unsigned long,void*);
extern int MI_SYS_AllocateMmapLayout(unsigned long,void*);
int JNI_OnLoad(void*a,void*b){char buf[96];int r,i;lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/sysjni.log",0x241,0644,0,0,0);put("start\n");r=MI_SYS_Init();put("init ");hx(r);
 for(i=0;i<3;i++){for(int j=0;j<96;j++)buf[j]=0;r=MI_SYS_GetMmapLayout(i,buf);put("get ");hx(i);put(" r ");hx(r);if(r==0){put("size ");hx(*(unsigned long*)(buf+16));put("phys ");hx(*(unsigned long*)(buf+24));}}
 for(i=0;i<3;i++){for(int j=0;j<96;j++)buf[j]=0;r=MI_SYS_AllocateMmapLayout(i,buf);put("alloc ");hx(i);put(" r ");hx(r);if(r==0){put("size ");hx(*(unsigned long*)(buf+16));put("phys ");hx(*(unsigned long*)(buf+24));}}
 put("end\n");return 0x10006;}
