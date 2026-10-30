#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
int main(int c,char**v){for(int i=1;i<c;i++){int fd=open(v[i],O_RDWR); printf("%s fd=%d errno=%d %s\n",v[i],fd,errno,strerror(errno)); if(fd>=0)close(fd);} }
