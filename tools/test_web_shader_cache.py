"""Exercise the complete web shader cache with fake GL and real temporary files."""
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
VERTEX, FRAGMENT = 0x8B31, 0x8B30
HEADER = r'''
#ifndef TEST_XGPU_H
#define TEST_XGPU_H
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef unsigned int GLenum;
typedef unsigned int GLuint;
#define GL_VERTEX_SHADER 0x8b31
#define GL_FRAGMENT_SHADER 0x8b30
GLuint xgpu_compile_shader(GLenum type, const char *source, const char *what);
const char *platform_save_root(void);
void platform_log(const char *format, ...);
#endif
'''
PREFIX = r'''
#include "xgpu.h"
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
static int compiled, linked, fail_allocation, fail_sync, forbid_main_io;
static pthread_t main_thread;
static const char *save_root;
const char *platform_save_root(void) { return save_root; }
void platform_log(const char *format, ...) { (void)format; }
GLuint xgpu_compile_shader(GLenum type, const char *source, const char *what)
{
    if (!pthread_equal(pthread_self(), main_thread) ||
        (type != GL_VERTEX_SHADER && type != GL_FRAGMENT_SHADER)) abort();
    (void)what;
    compiled++;
    return strcmp(source, "invalid") ? (GLuint)compiled : 0;
}
static void *cache_allocate(size_t size) { return fail_allocation ? NULL : malloc(size); }
static int cache_sync(int fd) { return fail_sync ? -1 : fsync(fd); }
static int cache_temporary(char *path)
{
    if (forbid_main_io && pthread_equal(pthread_self(), main_thread)) abort();
    return mkstemp(path);
}
static FILE *cache_open(const char *path, const char *mode)
{
    if (forbid_main_io && pthread_equal(pthread_self(), main_thread)) abort();
    return fopen(path, mode);
}
#define malloc cache_allocate
#define fsync cache_sync
#define fopen cache_open
#define mkstemp cache_temporary
'''
CASES = r'''
#undef malloc
#undef fsync
#undef fopen
#undef mkstemp
#define CHECK(test) do { if (!(test)) { fprintf(stderr, "failed line %d\n", __LINE__); return 1; } } while (0)
static void prepare_program(GLuint vertex, GLuint fragment)
{
    if (!pthread_equal(pthread_self(), main_thread)) abort();
    linked++;
    web_shader_cache_linked(vertex, fragment);
}
static void clear_memory(void)
{
    unsigned int i;
    for (i = 0; i < source_count; i++) free(sources[i]);
    memset(sources, 0, sizeof(sources));
    memset(source_buckets, 0, sizeof(source_buckets));
    memset(programs, 0, sizeof(programs));
    source_count = program_count = source_bytes = 0;
    compiled = linked = 0;
}
static void add_pair(void)
{
    GLuint vertex = web_shader_cache_get(GL_VERTEX_SHADER, "vertex source", "vertex");
    GLuint fragment = web_shader_cache_get(GL_FRAGMENT_SHADER, "fragment source", "pixel");
    web_shader_cache_linked(vertex, fragment);
    web_shader_cache_linked(vertex, fragment);
    web_shader_cache_linked(fragment, vertex);
}
int main(int argc, char **argv)
{
    GLuint first, second;
    if (argc != 3) return 2;
    main_thread = pthread_self();
    save_root = argv[2];
    snprintf(cache_path, sizeof(cache_path), "%s/web-shaders-v1.bin", save_root);
    snprintf(temporary_path, sizeof(temporary_path), "%s.XXXXXX", cache_path);
    if (!strcmp(argv[1], "reuse"))
    {
        char source[] = "void main() {}";
        first = web_shader_cache_get(GL_FRAGMENT_SHADER, source, "pixel");
        source[0] = 'X';
        second = web_shader_cache_get(GL_FRAGMENT_SHADER, "void main() {}", "pixel");
        CHECK(first && first == second && compiled == 1);
    }
    else if (!strcmp(argv[1], "stage"))
    {
        first = web_shader_cache_get(GL_VERTEX_SHADER, "void main() {}", "vertex");
        second = web_shader_cache_get(GL_FRAGMENT_SHADER, "void main() {}", "pixel");
        CHECK(first && second && first != second && compiled == 2);
        CHECK(web_shader_cache_get(GL_VERTEX_SHADER, "void main() {}", "vertex") == first);
    }
    else if (!strcmp(argv[1], "collision"))
    {
        const char *a = "COLLISION_A", *b = "COLLISION_B";
        CHECK(checksum_bytes(UINT32_C(2166136261) ^ GL_FRAGMENT_SHADER, a, strlen(a)) % CACHE_BUCKETS ==
              checksum_bytes(UINT32_C(2166136261) ^ GL_FRAGMENT_SHADER, b, strlen(b)) % CACHE_BUCKETS);
        first = web_shader_cache_get(GL_FRAGMENT_SHADER, a, "pixel");
        second = web_shader_cache_get(GL_FRAGMENT_SHADER, b, "pixel");
        CHECK(first && second && first != second && compiled == 2);
        CHECK(web_shader_cache_get(GL_FRAGMENT_SHADER, a, "pixel") == first);
        CHECK(web_shader_cache_get(GL_FRAGMENT_SHADER, b, "pixel") == second && compiled == 2);
    }
    else if (!strcmp(argv[1], "allocation"))
    {
        fail_allocation = 1;
        first = web_shader_cache_get(GL_FRAGMENT_SHADER, "allocation fallback", "pixel");
        CHECK(first && compiled == 1 && source_count == 0);
        fail_allocation = 0;
        second = web_shader_cache_get(GL_FRAGMENT_SHADER, "allocation fallback", "pixel");
        CHECK(second && compiled == 2);
        CHECK(web_shader_cache_get(GL_FRAGMENT_SHADER, "allocation fallback", "pixel") == second && compiled == 2);
    }
    else if (!strcmp(argv[1], "failure"))
    {
        CHECK(web_shader_cache_get(GL_FRAGMENT_SHADER, "invalid", "pixel") == 0 && compiled == 1);
        CHECK(web_shader_cache_get(GL_FRAGMENT_SHADER, "invalid", "pixel") == 0 && compiled == 1);
        CHECK(web_shader_cache_get(GL_FRAGMENT_SHADER, "valid", "pixel") != 0 && compiled == 2);
    }
    else if (!strcmp(argv[1], "roundtrip"))
    {
        add_pair();
        CHECK(source_count == 2 && program_count == 1);
        CHECK(cache_write(source_count, program_count));
        clear_memory();
        cache_load(prepare_program);
        CHECK(source_count == 2 && program_count == 1 && compiled == 2 && linked == 1);
        CHECK(web_shader_cache_get(GL_VERTEX_SHADER, "vertex source", "vertex") && compiled == 2);
        CHECK(web_shader_cache_get(GL_FRAGMENT_SHADER, "fragment source", "pixel") && compiled == 2);
    }
    else if (!strcmp(argv[1], "valid"))
    {
        cache_load(prepare_program);
        CHECK(source_count == 2 && program_count == 1 && compiled == 2 && linked == 1);
    }
    else if (!strcmp(argv[1], "invalid_file"))
    {
        cache_load(prepare_program);
        CHECK(source_count == 0 && program_count == 0 && compiled == 0 && linked == 0);
    }
    else if (!strcmp(argv[1], "atomic"))
    {
        add_pair();
        CHECK(cache_write(source_count, program_count));
        web_shader_cache_get(GL_FRAGMENT_SHADER, "another fragment", "pixel");
        fail_sync = 1;
        CHECK(!cache_write(source_count, program_count));
        fail_sync = 0;
        CHECK(access(temporary_path, F_OK) != 0);
        clear_memory();
        cache_load(prepare_program);
        CHECK(source_count == 2 && program_count == 1 && compiled == 2);
    }
    else if (!strcmp(argv[1], "bounds"))
    {
        source_bytes = CACHE_TOTAL_BYTES;
        first = web_shader_cache_get(GL_FRAGMENT_SHADER, "full", "pixel");
        CHECK(first && source_count == 0);
        source_bytes = 0;
        source_count = CACHE_SOURCES;
        second = web_shader_cache_get(GL_FRAGMENT_SHADER, "full", "pixel");
        CHECK(second && compiled == 2 && source_count == CACHE_SOURCES);
    }
    else if (!strcmp(argv[1], "worker"))
    {
        int i, saved = 0;
        struct timespec delay = {0, 10000000};
        web_shader_cache_initialize(prepare_program);
        CHECK(writer_enabled);
        forbid_main_io = 1;
        add_pair();
        web_shader_cache_initialize(prepare_program);
        CHECK(compiled == 2 && program_count == 1);
        for (i = 0; i < 400 && !saved; i++)
        {
            nanosleep(&delay, NULL);
            pthread_mutex_lock(&writer_lock);
            saved = generation && written_generation == generation;
            pthread_mutex_unlock(&writer_lock);
        }
        CHECK(saved && access(cache_path, F_OK) == 0);
    }
    else return 2;
    return 0;
}
'''


def checksum(data, seed=2166136261):
    for byte in data:
        seed = ((seed ^ byte) * 16777619) & 0xFFFFFFFF
    return seed


def cache_file(sources=((VERTEX, b'vertex source'), (FRAGMENT, b'fragment source')), pairs=((0, 1),)):
    data = b'OCEGLSC1' + struct.pack('<II', len(sources), len(pairs))
    for stage, source in sources:
        data += struct.pack('<II', stage, len(source)) + source
    data += b''.join(struct.pack('<II', *pair) for pair in pairs)
    return data + struct.pack('<I', checksum(data))


class CacheHarness(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix='halo-shader-cache-')
        cls.addClassCleanup(cls.directory.cleanup)
        cls.work = Path(cls.directory.name)
        (cls.work / 'xgpu.h').write_text(HEADER)
        cls.source = (ROOT / 'port/web/src/web_shader_cache.c').read_text()
        seen = {}
        for index in range(2000):
            text = f'void main() {{ fragment_color = vec4({index}.0); }}'
            bucket = checksum(text.encode(), 2166136261 ^ FRAGMENT) % 1024
            if bucket in seen:
                cls.cases = CASES.replace('COLLISION_A', seen[bucket]).replace('COLLISION_B', text)
                break
            seen[bucket] = text
        cls.executable = cls.build('cache', cls.source)

    @classmethod
    def build(cls, name, source):
        path = cls.work / (name + '.c')
        executable = cls.work / name
        path.write_text(PREFIX + source + cls.cases)
        subprocess.run([*shlex.split(os.environ.get('CC', 'clang')), '-std=gnu89', '-D_POSIX_C_SOURCE=200809L',
                        '-Wall', '-Wextra', '-Werror', '-pthread', '-I' + str(cls.work),
                        '-iquote', str(ROOT / 'port/linux/include'), str(path), '-o', str(executable)],
                       check=True, timeout=30)
        return executable

    def run_case(self, case, data=None, executable=None):
        with tempfile.TemporaryDirectory(prefix='halo-shader-cache-files-') as folder:
            if data is not None:
                (Path(folder) / 'web-shaders-v1.bin').write_bytes(data)
            result = subprocess.run([str(executable or self.executable), case, folder],
                                    capture_output=True, timeout=8)
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            if case == 'atomic':
                self.assertEqual({file.name for file in Path(folder).iterdir()}, {'web-shaders-v1.bin'})
            return (Path(folder) / 'web-shaders-v1.bin').read_bytes() if case == 'worker' else None


class ShaderPersistence(CacheHarness):
    def test_roundtrip_replays_sources_and_deduplicated_program_pairs(self): self.run_case('roundtrip')
    def test_valid_file_is_replayed_before_runtime_lookups(self): self.run_case('valid', cache_file())
    def test_failed_flush_preserves_previous_cache(self): self.run_case('atomic')
    def test_memory_budget_falls_back_to_normal_compilation(self): self.run_case('bounds')

    def test_corrupt_truncated_and_oversized_files_never_reach_gl(self):
        valid = cache_file()
        samples = [b'', valid[:7], valid[:-1], valid + b'extra', valid[:-4] + b'bad!',
                   b'OCEGLSC1' + struct.pack('<II', 1025, 0),
                   b'OCEGLSC1' + struct.pack('<II', 0, 4097),
                   cache_file(((123, b'unknown stage'),), ()),
                   cache_file(((VERTEX, b'embedded\0nul'),), ()),
                   cache_file(((VERTEX, b'x' * 65537),), ()),
                   cache_file(pairs=((2, 1),)), cache_file(pairs=((1, 0),))]
        for sample in samples:
            with self.subTest(size=len(sample)):
                self.run_case('invalid_file', sample)

    def test_worker_saves_without_main_thread_io_and_reload_is_valid(self):
        data = self.run_case('worker')
        self.run_case('valid', data)

    def test_negative_control_without_checksum_verification(self):
        bad = self.source.replace('saved_hash != hash || ', '')
        self.assertNotEqual(bad, self.source)
        executable = self.build('bad-checksum', bad)
        with tempfile.TemporaryDirectory() as folder:
            (Path(folder) / 'web-shaders-v1.bin').write_bytes(cache_file()[:-4] + b'bad!')
            result = subprocess.run([str(executable), 'invalid_file', folder], capture_output=True, timeout=5)
            self.assertNotEqual(result.returncode, 0)


if __name__ == '__main__':
    unittest.main()
