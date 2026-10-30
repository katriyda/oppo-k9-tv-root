#define SYS_open 5
#define SYS_write 4
#define SYS_close 6
static long sc(long n,long a,long b,long c,long d,long e,long f){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;register long r3 __asm__("r3")=d;register long r4 __asm__("r4")=e;register long r5 __asm__("r5")=f;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2),"r"(r3),"r"(r4),"r"(r5):"memory");return r0;}
static long lg=-1;
static void ini(void){lg=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/flashjni.log",0x241,0644,0,0,0);}
static void put(const char*s){int n=0;while(s[n])n++;if(lg>=0)sc(SYS_write,lg,(long)s,n,0,0,0);}
static void hx(unsigned long v){char b[12];int i;for(i=0;i<8;i++){int d=(v>>((7-i)*4))&15;b[i]=d<10?48+d:87+d;}b[8]=10;b[9]=0;put(b);}
static void z(char*p,int n){int i;for(i=0;i<n;i++)p[i]=0;}
extern int MI_FLASH_Init(void*);
extern int MI_FLASH_Open(void*, unsigned int*);
extern int MI_FLASH_GetHandle(void*, unsigned int*);
extern int MI_FLASH_Close(unsigned int);
int JNI_OnLoad(void* vm, void* res){char b[160];unsigned int h=0;int r,i;
 ini();put("onload2\n");
 r=MI_FLASH_Init(0);put("init ");hx(r);
 for(i=0;i<40;i++){z(b,160);*(unsigned int*)b=i;h=0;r=MI_FLASH_Open(b,&h);if(r==0){put("open idx ");hx(i);put(" h ");hx(h);MI_FLASH_Close(h);}}
 const char*names[4]={"MBOOT","boot","/dev/block/mmcblk0p9","system"};
 for(i=0;i<4;i++){z(b,160);int j=0;while(names[i][j]){b[j]=names[i][j];j++;}h=0;r=0xdead;r=MI_FLASH_Open(b,&h);put(names[i]);put(" r ");hx(r);put(" h ");hx(h);if(r==0)MI_FLASH_Close(h);}
 put("end\n");return 0x10006;}
