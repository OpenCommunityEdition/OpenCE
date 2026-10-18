#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
/* Darwin's host timeval headers may define FD_SETSIZE before game headers. */
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#include "engine_xdk_prefix.h"
extern unsigned long system_milliseconds(void);
extern unsigned long system_seconds(void);
extern int _stricmp(const char *,const char *);
static int checks,failures;
#define CHECK(x) do { ++checks;if(!(x)){++failures;fprintf(stderr,"FAIL line %d\n",__LINE__);} } while(0)
static volatile sig_atomic_t interrupted;
static void interrupt_sleep(int number){(void)number;interrupted=1;}
int main(void)
{
    LARGE_INTEGER frequency,start,end;
    unsigned long milliseconds;
    int i;time_t before,after;unsigned long wall;
    struct itimerval timer={{0,0},{0,5000}};
    before=time(NULL);wall=system_seconds();after=time(NULL);
    CHECK(wall>=(unsigned long)before&&wall<=(unsigned long)after);
    CHECK(!QueryPerformanceCounter(NULL));CHECK(!QueryPerformanceFrequency(NULL));
    CHECK(QueryPerformanceFrequency(&frequency));CHECK(frequency.QuadPart==1000000000LL);
    CHECK(QueryPerformanceCounter(&start));
    for(i=0;i<100;++i){CHECK(QueryPerformanceCounter(&end));CHECK(end.QuadPart>=start.QuadPart);start=end;}
    milliseconds=system_milliseconds();
    usleep(20000);CHECK((unsigned long)(system_milliseconds()-milliseconds)>=10);CHECK(QueryPerformanceCounter(&end));CHECK(end.QuadPart-start.QuadPart>=10000000LL);
    CHECK((unsigned long)(GetTickCount()-system_milliseconds())<2UL ||
          (unsigned long)(system_milliseconds()-GetTickCount())<2UL);
    Sleep(0);
    signal(SIGALRM,interrupt_sleep);
    CHECK(setitimer(ITIMER_REAL,&timer,NULL)==0);
    milliseconds=GetTickCount();Sleep(30);
    CHECK(interrupted);CHECK((unsigned long)(GetTickCount()-milliseconds)>=29UL);
    CHECK(_stricmp("BloodGulch","bloodgulch")==0);CHECK(_stricmp("a","B")<0);
    CHECK(_stricmp("prefix","prefix2")<0);CHECK(_stricmp("\377","\177")>0);
    printf("native clock and ASCII CRT: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
