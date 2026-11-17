/* SPDX-License-Identifier: MIT */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
static volatile sig_atomic_t stopping;
static void stop(int sig) { (void)sig; stopping=1; }
int main(int argc,char **argv) {
    int watch=argc==2 && !strcmp(argv[1],"--watch");
    if(argc>1 && !watch) { fprintf(stderr,"Usage: morse_hud_client [--watch]\n"); return 2; }
    signal(SIGINT,stop); signal(SIGTERM,stop);
    do {
        char buffer[4096];
        FILE *f=fopen("/run/morsemouse/status","r");
        if(!f) puts("MorseMouse: telemetry unavailable");
        else {
            size_t n=fread(buffer,1,sizeof(buffer)-1,f); int bad=ferror(f) || !feof(f);
            fclose(f); buffer[n]=0;
            unsigned long long stamp=0;
            char *s=strstr(buffer,"monotonic_ms="), *rate=strstr(buffer,"interval_ms=");
            unsigned interval=0;
            struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
            unsigned long long now=(unsigned long long)t.tv_sec*1000+t.tv_nsec/1000000;
            if(bad || strncmp(buffer,"version=1\n",10) || !s || sscanf(s,"monotonic_ms=%llu",&stamp)!=1 ||
                !rate || sscanf(rate,"interval_ms=%u",&interval)!=1 || interval<100 || interval>10000)
                puts("MorseMouse: invalid telemetry");
            else { printf("MorseMouse: %s\n",now<stamp || now-stamp>(unsigned long long)interval*3 ? "STALE":"current"); fputs(buffer,stdout); }
        }
        fflush(stdout);
        if(watch) { struct timespec wait={.tv_sec=1}; nanosleep(&wait,NULL); }
    } while(watch && !stopping);
    return 0;
}
