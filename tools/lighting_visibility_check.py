"""Compare packed and scalar reconstruction on the same live depth field."""
from harness import function


def instrument(text):
    body = function(text, 'reconstruct_visibility')
    packed = body.replace('reconstruct_visibility(', 'reconstruct_packed(', 1)
    scalar = body.replace('reconstruct_visibility(', 'reconstruct_scalar(', 1)
    scalar = scalar.replace('depth_visibility_row(', 'depth_visibility_scalar_row(')
    row = '''
static void depth_visibility_scalar_row(real depths[6][OCC_SIZE * OCC_SIZE], const real xs[OCC_SIZE],
    real y, real z, real extent, int resolution, real *output)
{
    int x;
    for (x = 0; x < OCC_SIZE; x++)
    {
        real_vector3d ray;
        ray.i = xs[x]; ray.j = y; ray.k = z;
        output[x] = depth_visibility(depths, &ray, extent, resolution);
    }
}
'''
    wrapper = r'''
static void reconstruct_visibility(real depths[6][OCC_SIZE * OCC_SIZE], real extent,
    int resolution, real dx, real dy, real dz, real *output)
{
    static FILE *file;
    static unsigned long calls, mismatches, last;
    static double packed_ticks, scalar_ticks;
    real reference[OCC_COUNT];
    LARGE_INTEGER a, b, c, frequency;
    unsigned long now;
    QueryPerformanceCounter(&a);
    if (calls & 1) reconstruct_packed(depths, extent, resolution, dx, dy, dz, output);
    else reconstruct_scalar(depths, extent, resolution, dx, dy, dz, reference);
    QueryPerformanceCounter(&b);
    if (calls & 1) reconstruct_scalar(depths, extent, resolution, dx, dy, dz, reference);
    else reconstruct_packed(depths, extent, resolution, dx, dy, dz, output);
    QueryPerformanceCounter(&c);
    packed_ticks += (double)((calls & 1) ? b.QuadPart-a.QuadPart : c.QuadPart-b.QuadPart);
    scalar_ticks += (double)((calls & 1) ? c.QuadPart-b.QuadPart : b.QuadPart-a.QuadPart);
    calls++;
    if (memcmp(output, reference, sizeof(reference))) mismatches++;
    if (!file)
    {
        file = fopen("visibility-check.csv", "w");
        if (file) fprintf(file, "milliseconds,fields,mismatches,packed_ms,scalar_ms\n");
    }
    now = GetTickCount();
    if (file && now-last >= 1000)
    {
        QueryPerformanceFrequency(&frequency);
        fprintf(file, "%lu,%lu,%lu,%.6f,%.6f\n", now, calls, mismatches,
            packed_ticks*1000/(double)frequency.QuadPart, scalar_ticks*1000/(double)frequency.QuadPart);
        fflush(file);
        last = now;
    }
}
'''
    return text.replace(body, '#include <stdio.h>\n'+row+packed+scalar+wrapper, 1)
