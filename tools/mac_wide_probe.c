#include "engine_wide.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#undef _wcsnicmp
extern int _wcsnicmp(const wchar_t *,const wchar_t *,size_t);
extern int _strnicmp(const char *,const char *,size_t);
extern char *_strdup(const char *);
#undef _vsnwprintf
extern int _vsnwprintf(wchar_t *, size_t, const wchar_t *, va_list);
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"line %d failed: %s\n",__LINE__,#x); } } while (0)
static int format(wchar_t *out, size_t count, const wchar_t *pattern, ...)
{
    int result;
    va_list args;
    va_start(args, pattern);
    result = _vsnwprintf(out, count, pattern, args);
    va_end(args);
    return result;
}
static void *tokenize_thread(void *unused)
{
    wchar_t text[] = L"other,thread";
    (void)unused;
    if (!wcstok(text,L",") || !wcstok(NULL,L",")) return (void *)1;
    return wcstok(NULL,L",");
}
int main(void)
{
    wchar_t output[256], token[] = L"one,two", *cursor;
    int written = 42;
    char *copy;
    pthread_t thread;
    void *thread_result = (void *)1;
    CHECK(sizeof(wchar_t) == 2);
    CHECK(_wcsnicmp(L"AbC",L"aBc",3)==0);
    CHECK(_wcsnicmp(L"ABz",L"abY",2)==0);
    CHECK(_wcsnicmp(L"ABz",L"abY",3)>0);
    CHECK(_wcsnicmp(L"\x03a9",L"\x03a8",1)>0);
    CHECK(_wcsnicmp(NULL,NULL,0)==0);
    CHECK(_strnicmp("ABz","abY",2)==0);
    CHECK(_strnicmp("ABz","abY",3)>0);
    CHECK(_strnicmp("","A",100)<0);
    CHECK(_strnicmp("\200","\177",1)>0);
    CHECK(_strnicmp(NULL,NULL,0)==0);
    copy=_strdup("Blood Gulch");CHECK(copy && !strcmp(copy,"Blood Gulch"));
    if(copy){copy[0]='b';CHECK(copy[0]=='b');free(copy);}

    CHECK(format(output,256,L"%s/%S/%hs/%ls",L"wide","narrow","small",L"large") == 23);
    CHECK(!wcscmp(output,L"wide/narrow/small/large"));
    CHECK(format(output,256,L"%d:%I64d:%g:%d:%I64X",7,-1234567890123LL,1.25,9,0x123456789abcdef0ULL) > 0);
    CHECK(!wcscmp(output,L"7:-1234567890123:1.25:9:123456789ABCDEF0"));
    CHECK(format(output,256,L"[%*.*s]/%u/%hd",-6,3,L"abcdef",0xffffffffU,-32769) > 0);
    CHECK(!wcscmp(output,L"[abc   ]/4294967295/32767"));
    CHECK(format(output,256,L"%c:%C:%hc:%lc",0xe9,'A','B',0x03a9) == 7);
    CHECK(output[0] == 0xe9 && output[6] == 0x03a9);
    CHECK(format(output,256,L"%s",L"\xd83d\xde80") == 2);
    CHECK(output[0] == 0xd83d && output[1] == 0xde80 && !output[2]);
    CHECK(format(output,4,L"abcdef") == -1);
    CHECK(!wcscmp(output,L"abc"));
    CHECK(format(output,256,L"A%nB",&written) == 2 && written == 42);
    CHECK(!wcscmp(output,L"AB"));
    cursor = wcstok(token,L","); CHECK(cursor && !wcscmp(cursor,L"one"));
    CHECK(pthread_create(&thread,NULL,tokenize_thread,NULL) == 0);
    CHECK(pthread_join(thread,&thread_result) == 0 && thread_result == NULL);
    cursor = wcstok(NULL,L","); CHECK(cursor && !wcscmp(cursor,L"two"));
    CHECK(wcstok(NULL,L",") == NULL);
    CHECK(format(output,256,L"%I64u",18446744073709551615ULL) == 20);
    CHECK(!wcscmp(output,L"18446744073709551615"));
    printf("native UTF16: %d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
