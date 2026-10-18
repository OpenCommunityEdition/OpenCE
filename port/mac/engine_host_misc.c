/* Tiger host services used by the real network and game diagnostics. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

/* The shared POSIX boundary uses a 32-bit size, including on PPC. Never
   substitute predictable bytes when the OS entropy source is unavailable. */
void posix_random_bytes(void *buffer, unsigned int size)
{
    unsigned char *cursor=(unsigned char *)buffer;
    int descriptor;
    ssize_t count;
    if(!size)return;
    if(!buffer)goto failed;
    do {descriptor=open("/dev/urandom",O_RDONLY);} while(descriptor<0 && errno==EINTR);
    if(descriptor<0)goto failed;
    if(fcntl(descriptor,F_SETFD,FD_CLOEXEC)<0){close(descriptor);goto failed;}
    while(size) {
        /* Bound each read to a positive signed count on the 32-bit ABI. */
        count=read(descriptor,cursor,size>65536U?65536U:size);
        if(count<0 && errno==EINTR)continue;
        if(count<=0){close(descriptor);goto failed;}
        cursor+=(size_t)count;size-=(unsigned int)count;
    }
    close(descriptor);return;
failed:
    fputs("native random source unavailable: cannot continue\n",stderr);abort();
}

/* WINAPI is empty on these native targets; LPCSTR is const char *. */
void OutputDebugStringA(const char *text)
{
    if(text)fputs(text,stderr);
}
