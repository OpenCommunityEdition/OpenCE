#ifndef HALO_MAC_ENGINE_WIDE_REDIRECT_H
#define HALO_MAC_ENGINE_WIDE_REDIRECT_H
/* Apply after native libc declarations. Keep Darwin's wint_t/wctype_t intact;
   the actual shared UTF-16 implementation takes unsigned-short code units.
   Routing and prototypes mirror port/linux/include/wchar.h. */
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
typedef char mac_wide_redirect_size[sizeof(wchar_t) == 2 ? 1 : -1];
#undef wcslen
#define wcslen msvc_wcslen
#undef wcsnlen
#define wcsnlen msvc_wcsnlen
#undef wcscpy
#define wcscpy msvc_wcscpy
#undef wcsncpy
#define wcsncpy msvc_wcsncpy
#undef wcscat
#define wcscat msvc_wcscat
#undef wcsncat
#define wcsncat msvc_wcsncat
#undef wcscmp
#define wcscmp msvc_wcscmp
#undef wcsncmp
#define wcsncmp msvc_wcsncmp
#undef wcscoll
#define wcscoll msvc_wcscoll
#undef wcsxfrm
#define wcsxfrm msvc_wcsxfrm
#undef wcschr
#define wcschr msvc_wcschr
#undef wcsrchr
#define wcsrchr msvc_wcsrchr
#undef wcsstr
#define wcsstr msvc_wcsstr
#undef wcsspn
#define wcsspn msvc_wcsspn
#undef wcscspn
#define wcscspn msvc_wcscspn
#undef wcspbrk
#define wcspbrk msvc_wcspbrk
#undef wcstok
#define wcstok msvc_wcstok
#undef wcstol
#define wcstol msvc_wcstol
#undef wcstoul
#define wcstoul msvc_wcstoul
#undef wcstod
#define wcstod msvc_wcstod
#undef wcsdup
#define wcsdup msvc_wcsdup
#undef _wcsdup
#define _wcsdup msvc_wcsdup
#undef _wcsicmp
#define _wcsicmp msvc_wcsicmp
#undef _wcsnicmp
#define _wcsnicmp msvc_wcsnicmp
#undef wcsicmp
#define wcsicmp msvc_wcsicmp
#undef wcsnicmp
#define wcsnicmp msvc_wcsnicmp
#undef _wcslwr
#define _wcslwr msvc_wcslwr
#undef _wcsupr
#define _wcsupr msvc_wcsupr
#undef wcslwr
#define wcslwr msvc_wcslwr
#undef wcsupr
#define wcsupr msvc_wcsupr
#undef _wtoi
#define _wtoi msvc_wtoi
#undef _wtol
#define _wtol msvc_wtol
#undef wmemchr
#define wmemchr msvc_wmemchr
#undef wmemcmp
#define wmemcmp msvc_wmemcmp
#undef wmemcpy
#define wmemcpy msvc_wmemcpy
#undef wmemmove
#define wmemmove msvc_wmemmove
#undef wmemset
#define wmemset msvc_wmemset
#undef towlower
#define towlower msvc_towlower
#undef towupper
#define towupper msvc_towupper
#undef iswalpha
#define iswalpha msvc_iswalpha
#undef iswupper
#define iswupper msvc_iswupper
#undef iswlower
#define iswlower msvc_iswlower
#undef iswdigit
#define iswdigit msvc_iswdigit
#undef iswxdigit
#define iswxdigit msvc_iswxdigit
#undef iswspace
#define iswspace msvc_iswspace
#undef iswpunct
#define iswpunct msvc_iswpunct
#undef iswalnum
#define iswalnum msvc_iswalnum
#undef iswprint
#define iswprint msvc_iswprint
#undef iswgraph
#define iswgraph msvc_iswgraph
#undef iswcntrl
#define iswcntrl msvc_iswcntrl
#undef iswascii
#define iswascii msvc_iswascii
#undef iswctype
#define iswctype msvc_iswctype
#undef swprintf
#define swprintf msvc_swprintf
#undef vswprintf
#define vswprintf msvc_vswprintf
#undef _snwprintf
#define _snwprintf msvc_snwprintf
#undef _vsnwprintf
#define _vsnwprintf msvc_vsnwprintf
#undef wprintf
#define wprintf msvc_wprintf
#undef vwprintf
#define vwprintf msvc_vwprintf
#undef fwprintf
#define fwprintf msvc_fwprintf
#undef vfwprintf
#define vfwprintf msvc_vfwprintf
#undef fgetwc
#define fgetwc msvc_fgetwc
#undef fputwc
#define fputwc msvc_fputwc
#undef getwc
#define getwc msvc_fgetwc
#undef putwc
#define putwc msvc_fputwc
#undef getwchar
#define getwchar() msvc_fgetwc(stdin)
#undef putwchar
#define putwchar(c) msvc_fputwc((c), stdout)
#undef ungetwc
#define ungetwc msvc_ungetwc
#undef fgetws
#define fgetws msvc_fgetws
#undef fputws
#define fputws msvc_fputws
#undef _getws
#define _getws msvc_getws
#undef _putws
#define _putws msvc_putws
#undef _wremove
#define _wremove msvc_wremove
#undef _wtmpnam
#define _wtmpnam msvc_wtmpnam
#undef _wctime
#define _wctime msvc_wctime
#undef _wasctime
#define _wasctime msvc_wasctime
#undef _wperror
#define _wperror msvc_wperror
#undef _wcserror
#define _wcserror msvc_wcserror
#undef _wfopen
#define _wfopen msvc_wfopen

size_t msvc_wcslen(const wchar_t *string);
size_t msvc_wcsnlen(const wchar_t *string, size_t maximum_count);
wchar_t *msvc_wcscpy(wchar_t *destination, const wchar_t *source);
wchar_t *msvc_wcsncpy(wchar_t *destination, const wchar_t *source, size_t count);
wchar_t *msvc_wcscat(wchar_t *destination, const wchar_t *source);
wchar_t *msvc_wcsncat(wchar_t *destination, const wchar_t *source, size_t count);
int msvc_wcscmp(const wchar_t *string1, const wchar_t *string2);
int msvc_wcsncmp(const wchar_t *string1, const wchar_t *string2, size_t count);
int msvc_wcscoll(const wchar_t *string1, const wchar_t *string2);
size_t msvc_wcsxfrm(wchar_t *destination, const wchar_t *source, size_t count);
wchar_t *msvc_wcschr(const wchar_t *string, wchar_t character);
wchar_t *msvc_wcsrchr(const wchar_t *string, wchar_t character);
wchar_t *msvc_wcsstr(const wchar_t *string, const wchar_t *substring);
size_t msvc_wcsspn(const wchar_t *string, const wchar_t *characters);
size_t msvc_wcscspn(const wchar_t *string, const wchar_t *characters);
wchar_t *msvc_wcspbrk(const wchar_t *string, const wchar_t *characters);
wchar_t *msvc_wcstok(wchar_t *string, const wchar_t *delimiters);
long msvc_wcstol(const wchar_t *string, wchar_t **end, int base);
unsigned long msvc_wcstoul(const wchar_t *string, wchar_t **end, int base);
double msvc_wcstod(const wchar_t *string, wchar_t **end);
wchar_t *msvc_wcsdup(const wchar_t *string);
int msvc_wcsicmp(const wchar_t *string1, const wchar_t *string2);
int msvc_wcsnicmp(const wchar_t *string1, const wchar_t *string2, size_t count);
wchar_t *msvc_wcslwr(wchar_t *string);
wchar_t *msvc_wcsupr(wchar_t *string);
int msvc_wtoi(const wchar_t *string);
long msvc_wtol(const wchar_t *string);
wchar_t *msvc_wmemchr(const wchar_t *buffer, wchar_t character, size_t count);
int msvc_wmemcmp(const wchar_t *buffer1, const wchar_t *buffer2, size_t count);
wchar_t *msvc_wmemcpy(wchar_t *destination, const wchar_t *source, size_t count);
wchar_t *msvc_wmemmove(wchar_t *destination, const wchar_t *source, size_t count);
wchar_t *msvc_wmemset(wchar_t *buffer, wchar_t character, size_t count);

unsigned short msvc_towlower(unsigned short character);
unsigned short msvc_towupper(unsigned short character);
int msvc_iswalpha(unsigned short character);
int msvc_iswupper(unsigned short character);
int msvc_iswlower(unsigned short character);
int msvc_iswdigit(unsigned short character);
int msvc_iswxdigit(unsigned short character);
int msvc_iswspace(unsigned short character);
int msvc_iswpunct(unsigned short character);
int msvc_iswalnum(unsigned short character);
int msvc_iswprint(unsigned short character);
int msvc_iswgraph(unsigned short character);
int msvc_iswcntrl(unsigned short character);
int msvc_iswascii(unsigned short character);
int msvc_iswctype(unsigned short character, unsigned short type);

int msvc_swprintf(wchar_t *buffer, const wchar_t *format, ...);
int msvc_vswprintf(wchar_t *buffer, const wchar_t *format, va_list arguments);
int msvc_snwprintf(wchar_t *buffer, size_t count, const wchar_t *format, ...);
int msvc_vsnwprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list arguments);
int msvc_wprintf(const wchar_t *format, ...);
int msvc_vwprintf(const wchar_t *format, va_list arguments);
int msvc_fwprintf(FILE *stream, const wchar_t *format, ...);
int msvc_vfwprintf(FILE *stream, const wchar_t *format, va_list arguments);

unsigned short msvc_fgetwc(FILE *stream);
unsigned short msvc_fputwc(wchar_t character, FILE *stream);
unsigned short msvc_ungetwc(unsigned short character, FILE *stream);
wchar_t *msvc_fgetws(wchar_t *string, int count, FILE *stream);
int msvc_fputws(const wchar_t *string, FILE *stream);
wchar_t *msvc_getws(wchar_t *string);
int msvc_putws(const wchar_t *string);
int msvc_wremove(const wchar_t *path);
wchar_t *msvc_wtmpnam(wchar_t *string);
wchar_t *msvc_wctime(const time_t *timer);
wchar_t *msvc_wasctime(const struct tm *time);
void msvc_wperror(const wchar_t *string);
wchar_t *msvc_wcserror(int error_number);
FILE *msvc_wfopen(const wchar_t *path, const wchar_t *mode);


#endif
