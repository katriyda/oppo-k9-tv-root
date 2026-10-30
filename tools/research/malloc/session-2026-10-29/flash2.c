#define SYS_open 5
#define SYS_write 4
#define SYS_close 6
static long sc(long n,long a,long b,long c){register long r7 __asm__("r7")=n;register long r0 __asm__("r0")=a;register long r1 __asm__("r1")=b;register long r2 __asm__("r2")=c;__asm__ volatile("svc 0":"+r"(r0):"r"(r7),"r"(r1),"r"(r2):"memory");return r0;}
static void lg(const char*s){long f=sc(SYS_open,(long)"/data/data/com.cvte.factory.service/files/flashlog",0x241,0644);if(f>=0){int n=0;while(s[n])n++;sc(SYS_write,f,(long)s,n);sc(SYS_close,f,0,0);}}
static void hx(unsigned v){char b[16];int i;for(i=0;i<8;i++){int d=(v>>((7-i)*4))&15;b[i]=d<10?48+d:87+d;}b[8]=10;b[9]=0;lg(b);}
extern int MI_FLASH_Init(void*);
extern int MI_FLASH_GetHandle(void*,int*);
extern int MI_FLASH_Open(void*,int*);
__attribute__((constructor)) static void init(void){lg("f2\n");int h=-1,r;
 r=MI_FLASH_Init(0);lg("I ");hx(r);
 unsigned char a[0x88];for(int i=0;i<0x88;i++)a[i]=0;
 a[0]=98;a[1]=111;a[2]=111;a[3]=116; // boot
 h=-1;r=MI_FLASH_GetHandle(a,&h);lg("G ");hx(r);hx(h);
 h=-1;r=MI_FLASH_Open(a,&h);lg("O ");hx(r);hx(h);
}
