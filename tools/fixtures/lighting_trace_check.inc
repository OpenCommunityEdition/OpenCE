/* Same rays, pose, flags and ordering for both paths. No simulation step can
   intervene. Clocks surround whole batches; output comparison is not timed. */
#include <stdio.h>
int lighting_trace_uncached;
static void trace_depths(real extent, real_point3d const *from, long ignore, int vehicles,
	real depths[6][OCC_SIZE * OCC_SIZE])
{
	static FILE *file;
	static unsigned long rays, mismatches, last, batches;
	static double fast_ticks, stock_ticks, uncached_ticks;
	static unsigned long cache_mismatches;
	real reference[6][OCC_SIZE * OCC_SIZE];
	real uncached[6][OCC_SIZE * OCC_SIZE];
	LARGE_INTEGER begin, middle, end, after_uncached, frequency;
	unsigned long now;
	int face, cell;
	QueryPerformanceCounter(&begin);
	trace_depths_actual(extent, from, ignore, vehicles, depths);
	QueryPerformanceCounter(&middle);
	lighting_trace_uncached = 1;
	trace_depths_actual(extent, from, ignore, vehicles, uncached);
	lighting_trace_uncached = 0;
	QueryPerformanceCounter(&after_uncached);
	trace_depths_reference(extent, from, ignore, vehicles, reference);
	QueryPerformanceCounter(&end);
	fast_ticks += (double)(middle.QuadPart-begin.QuadPart);
	uncached_ticks += (double)(after_uncached.QuadPart-middle.QuadPart);
	stock_ticks += (double)(end.QuadPart-after_uncached.QuadPart);
	batches++;
	for (face = 0; face < 6; face++)
		for (cell = 0; cell < OCC_SIZE * OCC_SIZE; cell++)
		{
			rays++;
			cache_mismatches += depths[face][cell] != uncached[face][cell];
			if (depths[face][cell] != reference[face][cell])
			{
				mismatches++;
				if (mismatches <= 10) fprintf(stderr, "TRACE mismatch %.9g %.9g face %d cell %d\n",
					depths[face][cell], reference[face][cell], face, cell);
			}
		}
	if (!file)
	{
		file = fopen("trace-check.csv", "w");
		if (file) fprintf(file, "milliseconds,rays,mismatches,batches,fast_ms,stock_ms,uncached_ms,cache_mismatches\n");
	}
	now = GetTickCount();
	if (file && now-last >= 1000)
	{
		QueryPerformanceFrequency(&frequency);
		fprintf(file, "%lu,%lu,%lu,%lu,%.6f,%.6f,%.6f,%lu\n", now, rays, mismatches, batches,
			fast_ticks*1000/(double)frequency.QuadPart, stock_ticks*1000/(double)frequency.QuadPart,
			uncached_ticks*1000/(double)frequency.QuadPart, cache_mismatches);
		fflush(file);
		last = now;
	}
}
