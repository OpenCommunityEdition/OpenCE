#include "engine_format.h"
#include <stdio.h>
#include <string.h>
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"line %d failed: %s\n",__LINE__,#x); } } while (0)
static int call_va(char *out, size_t count, const char *pattern, ...)
{
    va_list args;
    int result;
    va_start(args,pattern); result = _vsnprintf(out,count,pattern,args); va_end(args);
    return result;
}
static int call_shared_va(char *out, size_t count, const char *pattern, ...)
{
    va_list args; int result;
    va_start(args,pattern); result=halo_linux_vsnprintf(out,count,pattern,args); va_end(args);
    return result;
}
static int call_file_va(FILE *stream, const char *pattern, ...)
{
    va_list args; int result;
    va_start(args,pattern); result=halo_linux_vfprintf(stream,pattern,args); va_end(args);
    return result;
}
int main(void)
{
    char out[4096], pattern[2048], guard[8];
    FILE *stream;
    CHECK(sizeof(long) == 4 && sizeof(void *) == 4);
    CHECK(call_va(out,sizeof(out),"%d:%I64d:%.2f:%I32u:%I64X",7,-1234567890123LL,1.25,9U,0x123456789abcdef0ULL) == 40);
    CHECK(!strcmp(out,"7:-1234567890123:1.25:9:123456789ABCDEF0"));
    CHECK(_snprintf(out,sizeof(out),"[%*.*I64d] %%I64d %I32d",8,5,42LL,-7) == 19);
    CHECK(!strcmp(out,"[   00042] %I64d -7"));
    CHECK(_snprintf(out,4,"abcdef") == 6 && !strcmp(out,"abc"));
    memset(guard,'x',sizeof(guard));
    CHECK(_snprintf(guard+2,1,"%I64u",18446744073709551615ULL) == 20);
    CHECK(guard[1] == 'x' && guard[2] == 0 && guard[3] == 'x');
    CHECK(_snprintf(NULL,0,"%I64u/%I32d",18446744073709551615ULL,-2) == 23);
    CHECK(_snprintf(out,sizeof(out),"%% %I32x %08I64X",0xffffffffU,0x123ULL) == 19);
    CHECK(!strcmp(out,"% ffffffff 00000123"));
    memset(pattern,'a',1500); strcpy(pattern+1500,"-%I64d-%I32d");
    CHECK(_snprintf(out,sizeof(out),pattern,1234567890123LL,7) == 1516);
    CHECK(strlen(out) == 1516 && !strcmp(out+1500,"-1234567890123-7"));
    CHECK(_snprintf(out,sizeof(out),"%%I32 %%I64 %d",5) == 11);
    CHECK(!strcmp(out,"%I32 %I64 5"));
    CHECK(halo_linux_snprintf(out,sizeof(out),"%d:%I64d:%.2f:%I32u:%I64X",7,-1234567890123LL,1.25,9U,0x123456789abcdef0ULL)==40);
    CHECK(!strcmp(out,"7:-1234567890123:1.25:9:123456789ABCDEF0"));
    CHECK(call_shared_va(out,sizeof(out),"[%*.*I64d] %%I64d %I32d",8,5,42LL,-7)==19);
    CHECK(!strcmp(out,"[   00042] %I64d -7"));
    CHECK(halo_linux_snprintf(out,4,"abcdef")==6 && !strcmp(out,"abc"));
    CHECK(call_shared_va(NULL,0,"%I64u/%I32d",18446744073709551615ULL,-2)==23);
    stream=tmpfile();
    CHECK(stream!=NULL);
    if(stream) {
        CHECK(halo_linux_fprintf(stream,"%I64d:%I32u",-1234567890123LL,9U)==16);
        CHECK(call_file_va(stream,":%I64X",0x123456789abcdef0ULL)==17);
        rewind(stream);
        CHECK(fgets(out,sizeof(out),stream)!=NULL && !strcmp(out,"-1234567890123:9:123456789ABCDEF0"));
        fclose(stream);
    }
    printf("native narrow format: %d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
