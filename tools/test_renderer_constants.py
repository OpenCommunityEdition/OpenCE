"""Exercise the renderer's real constant tracking with recorded uniform uploads.

No game assets, GL context, or profile are needed. The timeout also catches
the captured ULONG_MAX infinite loop. Windows compiles with 32-bit long.
"""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def block(source, marker):
    start = source.index(marker)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


PRELUDE = r'''
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned int GLuint;
typedef int GLint, GLsizei, BOOL;
#define TRUE 1
#define FALSE 0
#define XGPU_VERTEX_CONSTANT_COUNT 192
#define PROGRAM_BUCKETS 1024
struct draw_uniforms { float unused[4]; };
static struct { float constants[XGPU_VERTEX_CONSTANT_COUNT][4]; } device;
static float gpu[4][XGPU_VERTEX_CONSTANT_COUNT][4];
static unsigned uploads;
static int upload_first, upload_count;
static void glUniform4fv(GLint location, GLsizei count, const float *values)
{
    int program = location / 512, first = location % 512;
    if (program < 0 || program >= 4 || first + count > XGPU_VERTEX_CONSTANT_COUNT)
        abort();
    memcpy(gpu[program][first], values, count * sizeof(gpu[0][0]));
    uploads++; upload_first = first; upload_count = count;
}
'''

TESTS = r'''
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); exit(1); } } while (0)
static struct program_entry programs[4];
static void reset(void)
{
    memset(&device, 0, sizeof(device)); memset(programs, 0, sizeof(programs));
    memset(program_buckets, 0, sizeof(program_buckets)); memset(gpu, 0, sizeof(gpu));
    memset(constant_serials, 0, sizeof(constant_serials)); memset(constant_log, 0, sizeof(constant_log));
    constants_serial = 0; uploads = 0;
    for (int p = 0; p < 4; p++) {
        programs[p].constants = p * 512;
        programs[p].constant_count = XGPU_VERTEX_CONSTANT_COUNT;
        programs[p].constants_consecutive = TRUE;
    }
    /* Multiple entries in one bucket, plus another bucket and a fresh program. */
    program_buckets[0] = &programs[0]; programs[0].next = &programs[1];
    program_buckets[PROGRAM_BUCKETS - 1] = &programs[2];
}
static void store(unsigned long index, float value)
{
    float values[4] = {value, value + 1, value + 2, value + 3};
    constants_store(index, values, 1);
}
static void draw(int p)
{
    upload_constants(&programs[p]);
    for (unsigned long i = 0; i < programs[p].constant_count; i++)
        CHECK(!memcmp(gpu[p][i], device.constants[i], sizeof(gpu[p][i])));
}
static void captured_hang(void)
{
    reset();
    constants_serial = ULONG_MAX - 2;
    programs[0].constants_serial = constants_serial;
    store(5, 5); store(9, 9);
    CHECK(constants_serial == ULONG_MAX);
    draw(0);
    CHECK(uploads == 1 && upload_first == 5 && upload_count == 5);
    CHECK(programs[0].constants_serial == ULONG_MAX);
    draw(0); CHECK(uploads == 1);
}
static void rollover(void)
{
    reset();
    programs[1].constant_count = 12; programs[2].constants_consecutive = FALSE;
    store(3, 3); store(7, 7); store(180, 180);
    draw(0); draw(1); draw(2);
    /* Leave an inactive program stale before the reset. */
    store(7, 77); draw(0);
    constants_serial = ULONG_MAX;
    programs[0].constants_serial = ULONG_MAX;
    store(3, 33);
    CHECK(constants_serial == 2);
    CHECK(programs[0].constants_serial == 0 && programs[1].constants_serial == 0 && programs[2].constants_serial == 0);
    for (unsigned long i = 0; i < XGPU_VERTEX_CONSTANT_COUNT; i++)
        CHECK(constant_serials[i] == (i == 3 ? 2 : 1));
    draw(0); CHECK(upload_first == 0 && upload_count == 192);
    draw(1); CHECK(upload_first == 0 && upload_count == 12);
    draw(2); CHECK(upload_first == 0 && upload_count == 192);
    /* A newly linked program must also see unchanged pre-reset constants. */
    draw(3);
    program_buckets[1] = &programs[3];
    unsigned before = uploads;
    draw(0); draw(1); draw(2); draw(3); CHECK(uploads == before);
    store(3, 333); draw(0); CHECK(upload_first == 3 && upload_count == 1);
    draw(1); draw(2); draw(3);
    /* Repeated rollover, including several registers in one store. */
    for (int repeat = 0; repeat < 3; repeat++) {
        float values[3][4] = {{1 + repeat, 2, 3, 4}, {5 + repeat, 6, 7, 8}, {9 + repeat, 10, 11, 12}};
        constants_serial = ULONG_MAX - 1;
        constants_store(2, values, 3);
        CHECK(constants_serial == 3);
        draw(0); draw(1); draw(2); draw(3);
    }
}
static void ordinary_updates(void)
{
    reset(); draw(0); CHECK(uploads == 0);
    store(5, 5); draw(0); CHECK(uploads == 1 && upload_count == 1);
    unsigned long serial = constants_serial;
    store(5, 5); CHECK(constants_serial == serial); draw(0); CHECK(uploads == 1);
    programs[1].constant_count = 8; draw(1);
    unsigned before = uploads;
    store(180, 1); draw(1); CHECK(uploads == before);
    /* Both full-scan paths: irrelevant changes, and a changed used register. */
    for (int i = 0; i < 250; i++) store(180, 2 + i);
    draw(1); CHECK(uploads == before);
    for (int i = 0; i < 250; i++) store(180, 252 + i);
    store(3, 3); draw(1); CHECK(uploads == before + 1 && upload_count == 1);
    draw(0); draw(2); draw(3);
    /* Switch programs with overlapping changes and a partially used array. */
    for (unsigned i = 0; i < 4000; i++) {
        store((i * 53) % XGPU_VERTEX_CONSTANT_COUNT, (float)i + 1000);
        if (i % 3 == 0) draw(i % 4);
    }
    draw(0); draw(1); draw(2); draw(3);
    programs[3].constants = -1; before = uploads;
    store(0, 99999); upload_constants(&programs[3]); CHECK(uploads == before);
}
int main(void)
{
    captured_hang(); rollover(); ordinary_updates();
    printf("PASS: %zu-bit constant serials; captured hang, repeated rollover, stale/fresh programs, partial/nonconsecutive arrays and ordinary uploads\n", sizeof(unsigned long) * CHAR_BIT);
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    parser.add_argument('--source', type=Path, default=ROOT / 'port/linux/src/d3d8_gl.c')
    args = parser.parse_args()
    source = args.source.read_text()
    tracking = source[source.index('/* each vertex constant register'):source.index('static void viewport_update_constants')]
    program = block(source, 'struct program_entry\n{') + ';\n'
    upload = block(source, 'if (entry->constants >= 0 && entry->constants_serial != constants_serial)')
    unit = (PRELUDE + program + 'static struct program_entry *program_buckets[PROGRAM_BUCKETS];\n' +
            tracking + 'static void upload_constants(struct program_entry *entry) {\n' + upload + '\n}\n' + TESTS)
    compiler = [args.cc, '-std=c11', '-O2', '-fuse-ld=lld']
    if sys.platform == 'win32':
        compiler.append('--target=i686-pc-windows-msvc')
    with tempfile.TemporaryDirectory(prefix='halo-renderer-test-') as directory:
        path = Path(directory)
        (path / 'constants.c').write_text(unit)
        subprocess.run([*compiler, str(path / 'constants.c'), '-o', str(path / 'constants.exe')], check=True)
        subprocess.run([str(path / 'constants.exe')], check=True, timeout=10)


if __name__ == '__main__':
    main()
