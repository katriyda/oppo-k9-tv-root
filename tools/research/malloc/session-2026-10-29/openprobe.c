#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
int main(){const char*p[]={"/dev/miomap","/dev/mik!sys","/dev/system","/dev/localdimming","/dev/mik!os","/dev/mik!pm"};for(int i=0;i<6;i++){int fd=open(p[i],O_RDWR);printf("%s fd=%d errno=%d %s\n",p[i],fd,errno,strerror(errno));if(fd>=0)close(fd);}return 0;}
