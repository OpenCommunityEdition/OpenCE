#include "engine_wide.h"
/* The engine currently declares this MS name directly, without the routing
   header. Its va_list remains the native PPC/i386 ABI throughout. */
#undef _vsnwprintf
int _vsnwprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list args)
{
    return msvc_vsnwprintf(buffer, count, format, args);
}

/* Tiger has pthread TLS but no compiler TLS. Preserve the shared tokenizer's
   per-thread state instead of replacing __thread with process-global storage. */
#include <pthread.h>
#include <stdlib.h>
static pthread_key_t tokenizer_key;
static pthread_once_t tokenizer_once = PTHREAD_ONCE_INIT;
static void tokenizer_create_key(void)
{
    if (pthread_key_create(&tokenizer_key, free) != 0) abort();
}
wchar_t **mac_wide_tokenizer_state(void)
{
    wchar_t **state;
    if (pthread_once(&tokenizer_once, tokenizer_create_key) != 0) abort();
    state = pthread_getspecific(tokenizer_key);
    if (!state) {
        state = calloc(1, sizeof(*state));
        if (!state) abort();
        if (pthread_setspecific(tokenizer_key, state) != 0) { free(state); abort(); }
    }
    return state;
}

/* Some engine units declare the MS name directly rather than including the
   routing header. Keep this boundary on the shared UTF-16 implementation. */
#undef _wcsnicmp
int _wcsnicmp(const wchar_t *a,const wchar_t *b,size_t count)
{return msvc_wcsnicmp(a,b,count);}
