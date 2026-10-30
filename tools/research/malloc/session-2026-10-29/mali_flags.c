#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#define KBASE_IOCTL_TYPE 0x80
struct vc { uint16_t major, minor; };
#define KBASE_IOCTL_VERSION_CHECK _IOWR(KBASE_IOCTL_TYPE, 0, struct vc)
struct sf { uint32_t create_flags; };
#define KBASE_IOCTL_SET_FLAGS _IOW(KBASE_IOCTL_TYPE, 1, struct sf)
int main(){int fd=open("/dev/mali0",O_RDWR); if(fd<0){puts("open fail");return 1;} for(int f=0;f<32;f++){struct sf s={f}; errno=0; int r=ioctl(fd,KBASE_IOCTL_SET_FLAGS,&s); printf("flags %d -> %d errno %d\n",f,r,errno);} close(fd);}
