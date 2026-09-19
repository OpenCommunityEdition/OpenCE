/* Counter/tick reconciliation recovered from the January target.
 * Global names are descriptive; their original debug names are unavailable.
 */
#include "radplatform.h"
#include <limits.h>
#include "radtimer.h"

static unsigned long rad_timer_last= 0;
static LARGE_INTEGER rad_timer_origin= { 0 };
static LARGE_INTEGER rad_timer_frequency= { 0 };
static unsigned long rad_timer_last_tick= 0;

#pragma data_seg("BINKDATA")
static int rad_timer_needs_initialization= 1;
static unsigned long rad_timer_correction= 0;
#pragma data_seg()

#pragma code_seg("BINK")
unsigned long __stdcall RADTimerRead(
	void)
{
	LARGE_INTEGER counter;
	unsigned long current_tick;
	unsigned long elapsed;
	long discrepancy;

	if (rad_timer_needs_initialization)
	{
		rad_timer_needs_initialization= 0;
		if (QueryPerformanceFrequency(&rad_timer_frequency))
		{
			QueryPerformanceCounter(&rad_timer_origin);
			rad_timer_last= 0;
			rad_timer_last_tick= GetTickCount();
			return 0;
		}
		rad_timer_frequency.QuadPart= 0;
	}
	else if (rad_timer_frequency.QuadPart)
	{
		QueryPerformanceCounter(&counter);
		current_tick= GetTickCount();
		elapsed= (unsigned long)(((unsigned __int64)counter.QuadPart-
			(unsigned __int64)rad_timer_origin.QuadPart)*1000/
			(unsigned __int64)rad_timer_frequency.QuadPart)+rad_timer_correction;
		discrepancy= (long)((current_tick-rad_timer_last_tick)-(elapsed-rad_timer_last));
		/* BUG (original machine behavior): the x86 absolute-value sequence
		 * leaves LONG_MIN negative and skips correction. Preserve that outcome
		 * without calling C abs(LONG_MIN), which has undefined behavior.
		 */
		if (discrepancy != LONG_MIN && (discrepancy > 200 || discrepancy < -200))
		{
			rad_timer_correction += discrepancy;
			elapsed += discrepancy;
		}
		if (elapsed-rad_timer_last > 0xc0000000UL)
		{
			return rad_timer_last;
		}
		rad_timer_last_tick= current_tick;
		rad_timer_last= elapsed;
		return elapsed;
	}
	return GetTickCount();
}
