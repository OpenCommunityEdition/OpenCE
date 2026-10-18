/* The game's names for the C library functions that voice_audio_native.c calls
   (cseries.h and the port's stdio.h rename them), for the probes that link it
   without the game. Compiled without the game's prefix header. */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
long csstrcmp(const char *a,const char *b){return strcmp(a,b);}
void *csmemcpy(void *d,const void *s,unsigned long n){return memcpy(d,s,n);}
FILE *halo_linux_fopen(const char *path,const char *mode){return fopen(path,mode);}
int halo_linux_snprintf(char *buffer,unsigned long count,const char *format,...)
{
	va_list args;
	int result;
	va_start(args,format);
	result=vsnprintf(buffer,count,format,args);
	va_end(args);
	return result;
}
