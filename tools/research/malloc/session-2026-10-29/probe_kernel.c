#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <linux/perf_event.h>
static long sc(long n,long a,long b,long c,long d,long e,long f){register long x8 __asm__("x8")=n;register long x0 __asm__("x0")=a;register long x1 __asm__("x1")=b;register long x2 __asm__("x2")=c;register long x3 __asm__("x3")=d;register long x4 __asm__("x4")=e;register long x5 __asm__("x5")=f;__asm__ volatile("svc 0":"+r"(x0):"r"(x8),"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5):"memory");return x0;}
static void wr(const char*p,const char*s){int fd=open(p,O_WRONLY);if(fd<0){printf("open %s: %s\n",p,strerror(errno));return;}int r=write(fd,s,strlen(s));printf("write %s -> %d %s\n",p,r,r<0?strerror(errno):"");close(fd);}
int main(){setbuf(stdout,NULL);printf("uid=%d\n",getuid());errno=0;int r=unshare(CLONE_NEWUSER);printf("unshare user=%d errno=%s\n",r,strerror(errno));if(r==0){wr("/proc/self/setgroups","deny\n");char b[32];sprintf(b,"0 %d 1\n",getuid());wr("/proc/self/uid_map",b);sprintf(b,"0 %d 1\n",getgid());wr("/proc/self/gid_map",b);errno=0;r=unshare(CLONE_NEWNET);printf("unshare net=%d errno=%s\n",r,strerror(errno));}
 struct perf_event_attr pe;memset(&pe,0,sizeof(pe));pe.type=PERF_TYPE_SOFTWARE;pe.size=sizeof(pe);pe.config=PERF_COUNT_SW_CPU_CLOCK;pe.disabled=1;errno=0;long fd=sc(SYS_perf_event_open,(long)&pe,0,-1,-1,0,0);printf("perf_open=%ld errno=%s\n",fd,strerror(errno));
 errno=0;long b=sc(SYS_bpf,0,0,0,0,0,0);printf("bpf=%ld errno=%s\n",b,strerror(errno));
 errno=0;long u=sc(SYS_userfaultfd,0,0,0,0,0,0);printf("uffd=%ld errno=%s\n",u,strerror(errno));
 errno=0;long k=sc(SYS_keyctl,1,(long)"x",0,0,0,0);printf("keyctl=%ld errno=%s\n",k,strerror(errno));
 return 0;}
