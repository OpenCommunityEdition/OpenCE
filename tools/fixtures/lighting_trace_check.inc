/* Same rays and pose; no simulation step intervenes. Rotate timing order to
   balance cache warming. Compare outputs outside the measured intervals. */
#include <stdio.h>
int lighting_trace_uncached, lighting_trace_unprepared, lighting_trace_recursive;
static void trace_depths(real extent, real_point3d const *from, long ignore, int vehicles, int resolution,
	real depths[6][OCC_SIZE * OCC_SIZE])
{
	static FILE *file;
	static unsigned long rays, mismatches, last, batches;
	static double ticks[5];
	static unsigned long recursive_mismatches;
	static unsigned long cache_mismatches, unprepared_mismatches;
	real reference[6][OCC_SIZE * OCC_SIZE];
	real uncached[6][OCC_SIZE * OCC_SIZE];
	real unprepared[6][OCC_SIZE * OCC_SIZE];
	real recursive[6][OCC_SIZE * OCC_SIZE];
	LARGE_INTEGER begin, end, frequency;
	unsigned long now;
	int face, cell, index, phase;
	for (index = 0; index < 5; index++)
	{
		phase = (index + batches) % 5;
		QueryPerformanceCounter(&begin);
		switch (phase)
		{
		case 0: trace_depths_actual(extent, from, ignore, vehicles, resolution, depths); break;
		case 1:
			lighting_trace_recursive = 1;
			trace_depths_reference(extent, from, ignore, vehicles, resolution, reference);
			lighting_trace_recursive = 0;
			break;
		case 2:
			lighting_trace_uncached = 1;
			trace_depths_actual(extent, from, ignore, vehicles, resolution, uncached);
			lighting_trace_uncached = 0;
			break;
		case 3:
			lighting_trace_unprepared = 1;
			trace_depths_actual(extent, from, ignore, vehicles, resolution, unprepared);
			lighting_trace_unprepared = 0;
			break;
		case 4:
			lighting_trace_recursive = 1;
			trace_depths_actual(extent, from, ignore, vehicles, resolution, recursive);
			lighting_trace_recursive = 0;
			break;
		}
		QueryPerformanceCounter(&end);
		ticks[phase] += (double)(end.QuadPart-begin.QuadPart);
	}
	batches++;
	for (face = 0; face < 6; face++)
		for (cell = 0; cell < OCC_SIZE * resolution; cell++)
		{
			if (cell % OCC_SIZE >= resolution) continue;
			rays++;
			cache_mismatches += depths[face][cell] != uncached[face][cell];
			unprepared_mismatches += depths[face][cell] != unprepared[face][cell];
			recursive_mismatches += depths[face][cell] != recursive[face][cell];
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
		if (file) fprintf(file, "milliseconds,rays,mismatches,batches,fast_ms,stock_ms,uncached_ms,cache_mismatches,unprepared_ms,unprepared_mismatches,recursive_ms,recursive_mismatches\n");
	}
	now = GetTickCount();
	if (file && now-last >= 1000)
	{
		QueryPerformanceFrequency(&frequency);
		fprintf(file, "%lu,%lu,%lu,%lu,%.6f,%.6f,%.6f,%lu,%.6f,%lu,%.6f,%lu\n", now, rays, mismatches, batches,
			ticks[0]*1000/(double)frequency.QuadPart, ticks[1]*1000/(double)frequency.QuadPart,
			ticks[2]*1000/(double)frequency.QuadPart, cache_mismatches,
			ticks[3]*1000/(double)frequency.QuadPart, unprepared_mismatches,
			ticks[4]*1000/(double)frequency.QuadPart, recursive_mismatches);
		fflush(file);
		last = now;
	}
}
