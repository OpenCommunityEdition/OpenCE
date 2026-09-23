/*
TIMESET.C

The Xbox CRT's initial timezone state and abbreviated English calendar names.
The two timezone buffers are writable because the CRT replaces them after
reading the system timezone.
*/

#include <Time.h>

char const __dnames[24] = "SunMonTueWedThuFriSat";
char const __mnames[37] = "JanFebMarAprMayJunJulAugSepOctNovDec";

long _timezone = 8 * 60 * 60;
int _daylight = 1;
long _dstbias = -60 * 60;

static char tzstd[64] = "PST";
static char tzdst[64] = "PDT";

char *_tzname[2] = { tzstd, tzdst };
