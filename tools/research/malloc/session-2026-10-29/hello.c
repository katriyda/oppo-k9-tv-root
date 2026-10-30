#include <fcntl.h>
#include <unistd.h>
int main(){const char *p="/data/data/com.cvte.factory.service/files/hello";int f=open(p,O_WRONLY|O_CREAT|O_TRUNC,0644);if(f<0){p="/data/local/tmp/hello.out";f=open(p,O_WRONLY|O_CREAT|O_TRUNC,0666);}if(f>=0){write(f,"APPEXEC\n",8);close(f);}return 0;}
