"""Exercise startup vertex-shader warming without WebGL or game assets."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PREFIX = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned long DWORD;
typedef unsigned int GLuint;
#define TRUE 1
#define FALSE 0
struct vertex_shader_object {
    unsigned int *instructions;
    struct { int lights; } lighting;
    unsigned int packed_mask;
    GLuint shaders[2][2];
};
static unsigned int code;
static struct vertex_shader_object objects[3];
static struct {
    int gl_ready;
    struct vertex_shader_object *vertex_shader;
    unsigned int program_address;
    struct vertex_shader_object *program_slots[4];
} device;
static int calls, compiles, wrong_declaration;
static unsigned int variants;
static struct vertex_shader_object *vertex_shader_from_handle(DWORD handle)
{
    return handle > 0 && handle < 4 ? &objects[handle - 1] : NULL;
}
static GLuint vertex_shader_get(struct vertex_shader_object *object, int immediate, int lit)
{
    calls++;
    if (device.vertex_shader != object || device.vertex_shader->packed_mask != object->packed_mask)
        wrong_declaration++;
    variants |= 1U << (2 * immediate + lit);
    if (!object->shaders[immediate][lit]) object->shaders[immediate][lit] = ++compiles;
    return object->shaders[immediate][lit];
}
'''
CASES = r'''
#define CHECK(test) do { if (!(test)) { fprintf(stderr, "failed line %d\n", __LINE__); return 1; } } while (0)
int main(int argc, char **argv)
{
    struct vertex_shader_object *saved = &objects[2];
    if (argc != 2) return 2;
    objects[0].instructions = &code;
    objects[0].packed_mask = 0x0e;
    objects[1].instructions = &code;
    objects[1].packed_mask = 0x8e;
    objects[1].lighting.lights = 4;
    device.gl_ready = 1;
    device.vertex_shader = saved;
    device.program_address = 3;
    device.program_slots[3] = &objects[1];
    if (!strcmp(argv[1], "unlit"))
    {
        halo_vertex_shader_prepare(1);
        CHECK(calls == 2 && compiles == 2 && variants == 5);
        CHECK(!wrong_declaration && device.vertex_shader == saved);
        CHECK(device.program_address == 3 && device.program_slots[3] == &objects[1]);
        halo_vertex_shader_prepare(1);
        CHECK(calls == 4 && compiles == 2);
    }
    else if (!strcmp(argv[1], "lit"))
    {
        halo_vertex_shader_prepare(2);
        CHECK(calls == 4 && compiles == 4 && variants == 15);
        CHECK(!wrong_declaration && device.vertex_shader == saved);
    }
    else if (!strcmp(argv[1], "null_saved"))
    {
        device.vertex_shader = NULL;
        halo_vertex_shader_prepare(1);
        CHECK(calls == 2 && !wrong_declaration && device.vertex_shader == NULL);
    }
    else if (!strcmp(argv[1], "uninitialized"))
    {
        device.gl_ready = 0;
        halo_vertex_shader_prepare(1);
        CHECK(calls == 0 && device.vertex_shader == saved);
    }
    else if (!strcmp(argv[1], "invalid"))
    {
        halo_vertex_shader_prepare(0);
        halo_vertex_shader_prepare(99);
        halo_vertex_shader_prepare(3);
        CHECK(calls == 0 && device.vertex_shader == saved);
    }
    else return 2;
    return 0;
}
'''

class VertexPrewarm(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / 'port/linux/src/d3d8_gl.c').read_text()
        start = source.index('void halo_vertex_shader_prepare(')
        end = source.index('{', start) + 1
        depth = 1
        while depth:
            depth += (source[end] == '{') - (source[end] == '}')
            end += 1
        cls.helper = source[start:end]
        cls.temp = tempfile.TemporaryDirectory(prefix='halo-vertex-prewarm-')
        cls.addClassCleanup(cls.temp.cleanup)
        cls.directory = Path(cls.temp.name)
        cls.exe = cls.build('candidate', cls.helper)

    @classmethod
    def build(cls, name, helper):
        source = cls.directory / (name + '.c')
        output = cls.directory / name
        source.write_text(PREFIX + helper + CASES)
        subprocess.run([*shlex.split(os.environ.get('CC', 'clang')), '-std=gnu89', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(output)], check=True, timeout=30)
        return output

    def run_case(self, case):
        subprocess.run([str(self.exe), case], check=True, timeout=30)

    def test_unlit_compiles_two_variants_and_preserves_bindings(self): self.run_case('unlit')
    def test_lit_compiles_four_variants_with_own_declaration(self): self.run_case('lit')
    def test_saved_null_declaration_is_restored(self): self.run_case('null_saved')
    def test_no_context_does_not_compile(self): self.run_case('uninitialized')
    def test_invalid_and_declaration_only_handles_do_not_compile(self): self.run_case('invalid')

    def test_negative_control_without_restoration(self):
        helper = self.helper.replace('device.vertex_shader = saved;', '(void)saved;')
        self.assertNotEqual(helper, self.helper)
        exe = self.build('bad-restoration', helper)
        result = subprocess.run([str(exe), 'unlit'], capture_output=True, timeout=5)
        self.assertNotEqual(result.returncode, 0)

    def test_negative_control_without_declaration_binding(self):
        helper = self.helper.replace('device.vertex_shader = object;', '/* missing declaration binding */')
        self.assertNotEqual(helper, self.helper)
        exe = self.build('bad-declaration', helper)
        result = subprocess.run([str(exe), 'lit'], capture_output=True, timeout=5)
        self.assertNotEqual(result.returncode, 0)

if __name__ == '__main__': unittest.main()
