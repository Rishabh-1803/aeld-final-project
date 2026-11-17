/* SPDX-License-Identifier: MIT */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
static void pause_ms(long ms) { struct timespec t={ms/1000,ms%1000*1000000}; nanosleep(&t,NULL); }
static void check(int rc,const char *name) { if(rc<0) { perror(name); exit(1); } }
static void event(int fd,unsigned code,int value) {
    struct input_event e[2]={0};
    e[0].type=EV_KEY;e[0].code=code;e[0].value=value;
    e[1].type=EV_SYN;e[1].code=SYN_REPORT;
    if(write(fd,e,sizeof(e))!=(ssize_t)sizeof(e)) { perror("write event");exit(1); }
}
static void click(int fd,unsigned code) { event(fd,code,1);pause_ms(30);event(fd,code,0);pause_ms(30); }
int main(int argc,char **argv) {
    int teardown=argc==2 && !strcmp(argv[1],"--teardown");
    if(argc>1 && !teardown)return 2;
    int fd=open("/dev/uinput",O_WRONLY|O_CLOEXEC);check(fd,"uinput");
    check(ioctl(fd,UI_SET_EVBIT,EV_KEY),"EV_KEY");check(ioctl(fd,UI_SET_EVBIT,EV_ABS),"EV_ABS");
    for(unsigned c=BTN_LEFT;c<=BTN_MIDDLE;c++)check(ioctl(fd,UI_SET_KEYBIT,c),"keybit");
    for(unsigned c=ABS_X;c<=ABS_Y;c++) {
        check(ioctl(fd,UI_SET_ABSBIT,c),"absbit");
        struct uinput_abs_setup a={.code=c,.absinfo={.minimum=0,.maximum=65535}};
        check(ioctl(fd,UI_ABS_SETUP,&a),"abs setup");
    }
    struct uinput_setup setup={.id={.bustype=BUS_USB,.vendor=0x1209,.product=0x0008}};
    strcpy(setup.name,"Morse Stress Tablet");check(ioctl(fd,UI_DEV_SETUP,&setup),"setup");
    check(ioctl(fd,UI_DEV_CREATE),"create");pause_ms(500);
    event(fd,BTN_LEFT,1);pause_ms(100);
    size_t count=250000;struct input_event *burst=calloc(count,sizeof(*burst));
    if(!burst)return 1;
    for(size_t i=0;i<count;i++){burst[i].type=EV_KEY;burst[i].code=BTN_LEFT;burst[i].value=2;}
    cpu_set_t cpus;CPU_ZERO(&cpus);CPU_SET(0,&cpus);check(sched_setaffinity(0,sizeof(cpus),&cpus),"affinity");
    struct sched_param rt={.sched_priority=1},normal={0};
    check(sched_setscheduler(0,SCHED_FIFO,&rt),"FIFO");
    ssize_t bytes=write(fd,burst,count*sizeof(*burst));
    int destroyed=0;
    if(teardown){destroyed=ioctl(fd,UI_DEV_DESTROY);}
    check(sched_setscheduler(0,SCHED_OTHER,&normal),"normal scheduler");
    free(burst);
    if(bytes!=(ssize_t)(count*sizeof(struct input_event))){fprintf(stderr,"incomplete burst\n");return 1;}
    if(teardown)check(destroyed,"destroy under pressure");
    else {
        pause_ms(500);event(fd,BTN_LEFT,0);pause_ms(100);
        click(fd,BTN_LEFT);click(fd,BTN_RIGHT);click(fd,BTN_MIDDLE);pause_ms(200);
        check(ioctl(fd,UI_DEV_DESTROY),"destroy");
    }
    close(fd);printf("PASS: 250000-event %s completed\n",teardown?"teardown":"overflow/recovery");return 0;
}
