"""Compare specialized visibility conversion with its generic loop in one pose."""
def instrument(text):
    declaration = 'static __attribute__((noinline)) void *prepare_field_texture('
    assert text.count(declaration) == 1
    first = text.index(declaration)
    end = text.index('{', first) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    body = text[first:end]
    boundary = '\tif (!(field->uploaded_mask & (1 << pass))'
    start = body.index(boundary)
    calculation = body[:start]
    assert calculation.count(declaration) == 1
    calculation = calculation.replace(declaration, 'static __attribute__((noinline)) void check_texture_pixels(unsigned long *output, ')
    calculation = calculation.replace('\tunsigned long pixels[OCC_COUNT];', '\tunsigned long *pixels = output;')
    calculation += '}\n'
    generic = calculation.replace('check_texture_pixels(', 'check_texture_generic(', 1)
    generic = generic.replace('if (!pass && !directional)', 'if (FALSE)').replace('else if (pass && !directional && (scale == 0.5f || scale == 1.0f))', 'else if (FALSE)')
    wrapper = r'''
static void check_texture(struct occlusion_field *field, int pass, int directional, real const axis[3])
{
    static FILE *file;
    static unsigned long calls, mismatches, last;
    static double fast_ticks, generic_ticks;
    unsigned long fast[OCC_COUNT], generic[OCC_COUNT], now;
    LARGE_INTEGER a, b, c, frequency;
    /* Alternate ordering to balance instruction/data cache warming. No
       uploads or field changes occur inside either timed calculation. */
    QueryPerformanceCounter(&a);
    if (calls & 1) check_texture_pixels(fast, field, pass, directional, axis);
    else check_texture_generic(generic, field, pass, directional, axis);
    QueryPerformanceCounter(&b);
    if (calls & 1) check_texture_generic(generic, field, pass, directional, axis);
    else check_texture_pixels(fast, field, pass, directional, axis);
    QueryPerformanceCounter(&c);
    fast_ticks += (double)((calls & 1) ? b.QuadPart-a.QuadPart : c.QuadPart-b.QuadPart);
    generic_ticks += (double)((calls & 1) ? c.QuadPart-b.QuadPart : b.QuadPart-a.QuadPart);
    calls++;
    if (memcmp(fast, generic, sizeof(fast))) mismatches++;
    if (!file)
    {
        file = fopen("texture-check.csv", "w");
        if (file) fprintf(file, "milliseconds,calls,mismatches,fast_ms,generic_ms\n");
    }
    now = GetTickCount();
    if (file && now-last >= 1000)
    {
        QueryPerformanceFrequency(&frequency);
        fprintf(file, "%lu,%lu,%lu,%.6f,%.6f\n", now, calls, mismatches,
            fast_ticks*1000/(double)frequency.QuadPart, generic_ticks*1000/(double)frequency.QuadPart);
        fflush(file);
        last = now;
    }
}
'''
    # Run the comparison only on cache misses. Production remains the only
    # writer/uploader and its return/failure handling is unchanged.
    checked = body.replace('\tscale = (pass == 2', '\tcheck_texture(field, pass, directional, axis);\n\tscale = (pass == 2', 1)
    return text.replace(body, '#include <stdio.h>\n'+calculation+generic+wrapper+checked, 1)
