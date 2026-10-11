"""Exercise the renderer's real index-cache functions under ASan and UBSan.

Uses fake GL buffers; no browser, GPU, game data or copied implementation.
"""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]

PREFIX = r'''

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned short WORD;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef ptrdiff_t GLsizeiptr;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88e4
static struct { unsigned long frame; } device;
static struct { GLuint element_array_buffer; } gl_state;
struct fake_buffer { void *data; size_t size; int live; };
static struct fake_buffer gpu[16384];
static unsigned uploads, deletions, invalidations, binds, fail_gen, fail_malloc, fail_realloc;
static GLuint bound;
static void glGenBuffers(int count, GLuint *result) {
    unsigned i;
    assert(count == 1);
    if (fail_gen) { fail_gen = 0; *result = 0; return; }
    for (i=1; i<16384; i++) if (!gpu[i].live) { gpu[i].live=1; *result=i; return; }
    assert(0);
}
static void glDeleteBuffers(int count, const GLuint *ids) {
    GLuint id = *ids;
    assert(count == 1 && id && gpu[id].live);
    free(gpu[id].data); memset(&gpu[id], 0, sizeof(gpu[id]));
    if (bound == id) bound=0;
    deletions++;
}
static void state_element_array_buffer(GLuint id) {
    if (gl_state.element_array_buffer != id) {
        assert(id && gpu[id].live);
        bound = gl_state.element_array_buffer = id;
        binds++;
    }
}
static void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    assert(target == GL_ELEMENT_ARRAY_BUFFER && usage == GL_STATIC_DRAW);
    assert(bound && gpu[bound].live && !gpu[bound].data);
    assert(size > 0);
    gpu[bound].data=malloc(size); memcpy(gpu[bound].data,data,size);
    gpu[bound].size=size; uploads++;
}
static void vertex_array_cache_invalidate(void) { invalidations++; }
static void index_extent(const WORD *indices, unsigned long count, unsigned long generation, BOOL cached,
                         unsigned long *minimum, unsigned long *maximum) {
    unsigned long i;
    (void)generation; (void)cached;
    *minimum=65535; *maximum=0;
    for(i=0;i<count;i++) { if(indices[i]<*minimum)*minimum=indices[i]; if(indices[i]>*maximum)*maximum=indices[i]; }
}
static void *candidate_malloc(size_t size) { if(fail_malloc) { fail_malloc=0; return NULL; } return malloc(size); }
static void *candidate_realloc(void *ptr, size_t size) { if(fail_realloc) { fail_realloc=0; return NULL; } return realloc(ptr,size); }
#define malloc candidate_malloc
#define realloc candidate_realloc
'''

CASES = r'''

#undef malloc
#undef realloc
static void reset(void) {
    unsigned i;
    for(i=0;i<INDEX_CACHE_SLOTS;i++) { index_cache_release(&index_cache.entries[i]); free(index_cache.entries[i].copy); }
    assert(!index_cache.used);
    for(i=1;i<16384;i++) assert(!gpu[i].live);
    memset(&index_cache,0,sizeof(index_cache));
    uploads=deletions=invalidations=binds=fail_gen=fail_malloc=fail_realloc=0;
    bound=0; gl_state.element_array_buffer=(GLuint)-1; device.frame=10;
}
static void valid_payload(GLuint buffer, const WORD *indices, unsigned long count, unsigned long minimum) {
    unsigned long i;
    assert(gpu[buffer].live && gpu[buffer].size==count*sizeof(WORD));
    for(i=0;i<count;i++) assert(((WORD *)gpu[buffer].data)[i] == (WORD)(indices[i]-minimum));
}
int main(void) {
    WORD indices[3]={8,5,11};
    unsigned long lo,hi,i,j;
    GLuint id,old;
    WORD *arrays[5];
    reset();
    assert(!index_cache_get(indices,3,&lo,&hi,&id));
    assert(lo==5 && hi==11 && uploads==0);
    assert(index_cache_get(indices,3,&lo,&hi,&id)); old=id;
    assert(lo==5 && hi==11 && uploads==1 && index_cache.used==6);
    valid_payload(id,indices,3,lo);
    device.frame++;
    assert(index_cache_get(indices,3,&lo,&hi,&id) && id==old && uploads==1);
    indices[1]=4;
    assert(!index_cache_get(indices,3,&lo,&hi,&id));
    assert(lo==4 && hi==11 && deletions==1 && invalidations==1 && !index_cache.used);
    assert(gl_state.element_array_buffer==(GLuint)-1);
    assert(index_cache_get(indices,3,&lo,&hi,&id));
    assert(id==old && binds==2); /* the GL name was reused; it must be rebound */
    valid_payload(id,indices,3,lo);
    reset();
    /* Five identical hash buckets compete for four probe slots. */
    for(i=0;i<5;i++) {
        assert(!posix_memalign((void **)&arrays[i],16384,16384));
        arrays[i][0]=7; arrays[i][1]=12; arrays[i][2]=9;
        device.frame++;
        assert(!index_cache_get(arrays[i],3,&lo,&hi,&id));
        assert(index_cache_get(arrays[i],3,&lo,&hi,&id));
        valid_payload(id,arrays[i],3,lo);
    }
    assert(deletions==1 && index_cache.used==4*6);
    reset(); for(i=0;i<5;i++) free(arrays[i]);
    /* Distinct slots exceed the aggregate budget without an all-cache clear. */
    for(i=0;i<5;i++) {
        unsigned long count=1024*1024+i;
        assert(!posix_memalign((void **)&arrays[i],16384,count*sizeof(WORD)));
        for(j=0;j<count;j++) arrays[i][j]=(WORD)(j%100+20);
        assert(!index_cache_get(arrays[i],count,&lo,&hi,&id));
        assert(index_cache_get(arrays[i],count,&lo,&hi,&id));
        assert(index_cache.used<=INDEX_CACHE_BUFFER_SIZE);
        valid_payload(id,arrays[i],count,lo);
    }
    assert(deletions>0 && deletions<5 && index_cache.used>2*1024*1024);
    reset(); for(i=0;i<5;i++) free(arrays[i]);
    /* Oversized arrays retain correct extents and fall back to transient data. */
    arrays[0]=calloc(INDEX_CACHE_BUFFER_SIZE/sizeof(WORD)+1,sizeof(WORD));
    assert(!index_cache_get(arrays[0],INDEX_CACHE_BUFFER_SIZE/sizeof(WORD)+1,&lo,&hi,&id));
    assert(!index_cache_get(arrays[0],INDEX_CACHE_BUFFER_SIZE/sizeof(WORD)+1,&lo,&hi,&id));
    assert(!uploads && !index_cache.used); reset(); free(arrays[0]);
    /* Allocation errors leave the cache retryable and accounting intact. */
    fail_realloc=1; assert(!index_cache_get(indices,3,&lo,&hi,&id));
    assert(!index_cache.used && !uploads);
    assert(!index_cache_get(indices,3,&lo,&hi,&id));
    fail_malloc=1; assert(!index_cache_get(indices,3,&lo,&hi,&id));
    assert(!index_cache.used && !uploads);
    fail_gen=1; assert(!index_cache_get(indices,3,&lo,&hi,&id));
    assert(!index_cache.used && !uploads);
    assert(index_cache_get(indices,3,&lo,&hi,&id));
    valid_payload(id,indices,3,lo); reset();
    puts("PASS: cache hit, rebasing, mutation, name reuse, collision, bounded eviction, oversized fallback, allocation failures");
    return 0;
}
'''

class WebIndexCache(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / "port/linux/src/d3d8_gl.c").read_text()
        start = source.index("#define INDEX_CACHE_BUFFER_SIZE")
        end = source.index("\n#endif", start)
        cls.cache = source[start:end]
        cls.temporary = tempfile.TemporaryDirectory(prefix="halo-index-cache-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.directory = Path(cls.temporary.name)
        cls.executable = cls.build("cache", cls.cache)

    @classmethod
    def build(cls, name, cache):
        source = cls.directory / (name + ".c")
        executable = cls.directory / name
        source.write_text(PREFIX + cache + CASES)
        subprocess.run(
            [*shlex.split(os.environ.get("CC", "clang")),
             "-std=gnu89", "-Wall", "-Wextra", "-Werror", "-g",
             "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
             "-fno-sanitize-recover=all", str(source), "-o", str(executable)],
            check=True, timeout=30)
        return executable

    def test_lifecycle_payload_eviction_and_failure_paths_under_sanitizers(self):
        # The C cases exercise first miss, second upload, hits, rebasing,
        # mutation, GL name reuse, four-probe collisions, bounded eviction,
        # oversized fallback and malloc/realloc/glGenBuffers failures.
        subprocess.run([str(self.executable)], check=True, timeout=30)

    def test_negative_control_wrong_rebasing_is_detected(self):
        bad = self.cache.replace("indices[index] - entry->minimum",
                                 "indices[index] + entry->minimum")
        self.assertNotEqual(bad, self.cache)
        executable = self.build("bad-rebasing", bad)
        result = subprocess.run([str(executable)], capture_output=True, timeout=30)
        self.assertNotEqual(result.returncode, 0)

    def test_negative_control_deleted_name_shadow_is_detected(self):
        bad = self.cache.replace("gl_state.element_array_buffer = (GLuint)-1;",
                                 "(void)gl_state.element_array_buffer;")
        self.assertNotEqual(bad, self.cache)
        executable = self.build("bad-shadow", bad)
        result = subprocess.run([str(executable)], capture_output=True, timeout=30)
        self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
