/* Narrow CRT boundary for the native player-record integration slice. These
   implement memory/string/log services, never player or game behavior. */
#include "cseries.h"
#include <wchar.h>
#undef memcmp
#undef strncpy
char temporary[256];
/* players.c retains a constant debug descriptor referring to this scenario
   global even when spawn code is stripped. No scenario is active here. */
short global_structure_bsp_index=-1;
long csmemcmp(const void *a,const void *b,unsigned long size)
{ return memcmp(a,b,size); }
char *csstrncpy(char *out,const char *in,unsigned long count)
{
    if (!out || !in || count>=0x2000) abort();
    return strncpy(out,in,count);
}
char *csprintf(char *out,char *format,...)
{
    va_list args;
    va_start(args,format);
    vsnprintf(out,out==temporary?sizeof(temporary):0x2000,format,args);
    va_end(args);
    return out;
}
void platform_log(const char *format,...)
{
    va_list args;
    va_start(args,format);vfprintf(stderr,format,args);va_end(args);fputc('\n',stderr);
}
void system_exit(long code) { exit((int)code); }
wchar_t *halo_mac_utf16_wcsncpy(wchar_t *out,const wchar_t *in,size_t count)
{
    size_t i=0;
    while(i<count && in[i]) { out[i]=in[i];++i; }
    while(i<count)out[i++]=0;
    return out;
}
