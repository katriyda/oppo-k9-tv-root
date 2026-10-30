#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/ioctl.h>
typedef uint64_t u64; typedef uint32_t u32; typedef uint16_t u16;
#define KBASE_IOCTL_TYPE 0x80
struct vc{u16 a,b;}; struct sf{u32 f;};
union ma{struct{u64 v,c,e,f;}in;struct{u64 f,g;}out;};
struct ai{u64 h;u64 o;u64 l;};
union al{struct{u64 f,s,n,a;}in;struct{u64 f,g,v;}out;};
struct fca{u64 g;u64 f;u64 m;}; struct fcb{u64 g;u64 m;u64 f;};
#define P1 (1ULL<<0)
#define P2 (1ULL<<1)
#define G2 (1ULL<<2)
#define G3 (1ULL<<3)
#define SAME (1ULL<<13)
#define DN (1ULL<<17)
#define COH (1ULL<<10)|(1ULL<<11)
#define IOCTL_IOWR(nr,sz) (0xc0000000u|((sz)<<16)|(0x80<<8)|(nr))
#define IOCTL_IOW(nr,sz) (0x40000000u|((sz)<<16)|(0x80<<8)|(nr))
#define ALLOC (16u*1024*1024)
int main(){int fd=open("/dev/mali0",2);struct vc v={999,999};if(fd<0){perror("open");return 1;}if(ioctl(fd,IOCTL_IOWR(0,sizeof(struct vc)),&v)<0){perror("ver");return 1;}printf("ver %d.%d\n",v.a,v.b);struct sf sf={0};if(ioctl(fd,IOCTL_IOW(1,sizeof(sf)),&sf)<0)perror("set");void*t=mmap(0,ALLOC,3,1,fd,3ULL<<12);if(t==MAP_FAILED){perror("track");return 1;}union ma m;memset(&m,0,sizeof(m));m.in.v=ALLOC>>12;m.in.c=ALLOC>>12;m.in.f=SAME|P1|P2|G2|G3;if(ioctl(fd,IOCTL_IOWR(5,sizeof(m)),&m)<0){perror("alloc");return 1;}printf("gpuva=%llx\n",(unsigned long long)m.out.g);void*g=mmap(0,ALLOC,3,1,fd,(off_t)m.out.g);if(g==MAP_FAILED){perror("gmap");return 1;}printf("g=%p\n",g);union al a;memset(&a,0,sizeof(a));a.in.f=G2|G3|P1|P2;a.in.s=ALLOC>>12;a.in.n=1;a.in.a=(u64)(uintptr_t)&(struct ai){.h=(u64)(uintptr_t)g,.o=0,.l=ALLOC>>12};if(ioctl(fd,IOCTL_IOWR(21,sizeof(a)),&a)<0){perror("alias");return 1;}printf("alias=%llx\n",(unsigned long long)a.out.g);
 u64 gs[2]={(u64)(uintptr_t)g,m.out.g};
 for(int k=0;k<2;k++){u64 gv=gs[k];for(int l=0;l<2;l++){for(int j=0;j<4;j++){u64 mask=0,flags=0;if(j==0){mask=DN;flags=DN;}if(j==1){mask=DN|COH;flags=DN|COH;}if(j==2){mask=DN;flags=0;}if(j==3){mask=0;flags=0;}int r;errno=0;if(l==0){struct fca x={gv,flags,mask};r=ioctl(fd,IOCTL_IOW(23,sizeof(x)),&x);}else{struct fcb x={gv,mask,flags};r=ioctl(fd,IOCTL_IOW(23,sizeof(x)),&x);}printf("g%d l%d v%d r=%d errno=%d\n",k,l,j,r,errno);}}}
 return 0;}
