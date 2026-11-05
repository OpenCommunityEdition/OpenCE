"""Depth-state regression and optional real GPU checks (HALO_TEST_GPU=1).

The shader emitter and bias calculations come from the production sources.
GPU checks need GLFW, PyOpenGL and a hidden desktop OpenGL 4.5 context.
"""
import ctypes
import os
import re
import subprocess

import pytest

from harness import build, read
from harness.tests.test_plasma_model_shader import shader_source


@pytest.fixture(scope='module')
def emitter():
    return build('plasma_model_shader', (('under_test.inc', shader_source('desktop')),))


def emit(emitter, *args):
    return subprocess.check_output([emitter, *map(str, args)], text=True)


def test_fixed_function_depth_mode(emitter):
    normal = emit(emitter, 'fixed', 0)
    reversed_shader = emit(emitter, 'fixed', 1)
    assert 'depth_bias * gl_Position.w' not in normal
    assert 'gl_Position.w - gl_Position.z + depth_bias * gl_Position.w' in reversed_shader


def test_desktop_default():
    config = read('port/linux/src/port_config.c')
    assert re.search(r'"display.reversed_depth",\s*_config_boolean,\s*"true",'
                     r'\s*"HALO_REVERSED_DEPTH",\s*_environment_value,\s*_platform_desktop', config)


def test_bias_units_and_slope(tmp_path):
    renderer = read('port/linux/src/d3d8_gl.c')
    bias = re.search(r'converted->depth_bias = .*?;', renderer, re.S).group()
    start = renderer.index('offset[0] = dword_to_float(rs[D3DRS_POLYGONOFFSETZSLOPESCALE])')
    stop = renderer.index('if (memcmp(gl_state.polygon_offset', start)
    slope = renderer[start:stop]
    source = r'''
#include <stdio.h>
#include <string.h>
typedef unsigned int DWORD;
enum { D3DRS_SOLIDOFFSETENABLE, D3DRS_POLYGONOFFSETZOFFSET, D3DRS_POLYGONOFFSETZSLOPESCALE };
static float dword_to_float(DWORD word) { float value; memcpy(&value, &word, 4); return value; }
static DWORD bits(float value) { DWORD word; memcpy(&word, &value, 4); return word; }
int main(void)
{
    struct { float viewport_scale[4]; } device;
    struct { float depth_bias; } result, *converted = &result;
    DWORD D3D__RenderState[3], *rs = D3D__RenderState;
    float ranges[] = {1.0f, 0.1f, 0.75f, 0.0f};
    float scales[] = {1.0f, 1.5f, 2.25f, 4.0f};
    float offset[2], target_scale[2];
    int xgpu_reversed_depth, enabled, range, scale;
    rs[D3DRS_POLYGONOFFSETZOFFSET] = bits(-8.0f);
    rs[D3DRS_POLYGONOFFSETZSLOPESCALE] = bits(-1.0f);
    for (xgpu_reversed_depth = 0; xgpu_reversed_depth < 2; xgpu_reversed_depth++)
    for (enabled = 0; enabled < 2; enabled++)
    for (range = 0; range < 4; range++)
    for (scale = 0; scale < 4; scale++)
    {
        rs[D3DRS_SOLIDOFFSETENABLE] = enabled;
        device.viewport_scale[2] = 16777215.0f * ranges[range];
        target_scale[0] = 1.0f;
        target_scale[1] = scales[scale];
''' + bias + '\n' + slope + r'''
        printf("%d %d %.9g %.9g %.9g %.9g %.9g\n", xgpu_reversed_depth, enabled,
            ranges[range], scales[scale], converted->depth_bias, offset[0], offset[1]);
    }
    return 0;
}
'''
    path = tmp_path/'depth.c'
    path.write_text(source)
    binary = tmp_path/('depth.exe' if os.name == 'nt' else 'depth')
    subprocess.run(['clang', '-m32', '-std=gnu89', '-Wall', '-Werror', str(path), '-o', str(binary)], check=True)
    for line in subprocess.check_output([binary], text=True).splitlines():
        reversed_depth, enabled, span, scale, bias_value, slope_value, units = map(float, line.split())
        expected = 8/16777215 if reversed_depth and enabled and span else 0
        assert bias_value*span == pytest.approx(expected, rel=2e-7, abs=1e-14)
        assert slope_value == (scale if reversed_depth else -scale)
        assert units == (0 if reversed_depth else -8)


@pytest.fixture(scope='module')
def gpu():
    if os.environ.get('HALO_TEST_GPU') != '1':
        pytest.skip('Set HALO_TEST_GPU=1 to run hidden OpenGL checks')
    import glfw
    from OpenGL import GL
    assert glfw.init(), 'GLFW initialization failed'
    window = None
    try:
        glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
        glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 4)
        glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 5)
        glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
        window = glfw.create_window(16, 16, 'Depth regression', None, None)
        assert window, 'OpenGL 4.5 context unavailable'
        glfw.make_context_current(window)
        print('GPU:', GL.glGetString(GL.GL_RENDERER).decode())
        yield GL
    finally:
        if window:
            glfw.destroy_window(window)
        glfw.terminate()


def test_gpu_stock_shaders(emitter, gpu):
    from OpenGL.GL.shaders import compileShader
    for reversed_depth in (0, 1):
        for enabled in (0, 1):
            for index in range(67):
                for packed in (0, 1):
                    shader = compileShader(emit(emitter, 'translated', reversed_depth, index, enabled, packed),
                                           gpu.GL_VERTEX_SHADER)
                    gpu.glDeleteShader(shader)


def test_gpu_projection_precision(emitter, gpu):
    """The real projection program must separate distant, closely spaced surfaces."""
    from OpenGL.GL.shaders import compileShader
    gl = gpu
    shader = compileShader(emit(emitter, 'translated', 1, 0, 0, 0), gl.GL_VERTEX_SHADER)
    program = gl.glCreateProgram()
    buffer = gl.glGenBuffers(1)
    vao = gl.glGenVertexArrays(1)
    try:
        gl.glAttachShader(program, shader)
        names = (ctypes.c_char_p*1)(b'gl_Position')
        gl.glTransformFeedbackVaryings(program, 1,
            ctypes.cast(names, ctypes.POINTER(ctypes.POINTER(ctypes.c_char))), gl.GL_INTERLEAVED_ATTRIBS)
        gl.glLinkProgram(program)
        assert gl.glGetProgramiv(program, gl.GL_LINK_STATUS), gl.glGetProgramInfoLog(program)
        gl.glUseProgram(program)
        gl.glBindVertexArray(vao)
        gl.glBindBuffer(gl.GL_TRANSFORM_FEEDBACK_BUFFER, buffer)
        gl.glBufferData(gl.GL_TRANSFORM_FEEDBACK_BUFFER, 16, None, gl.GL_STREAM_READ)
        gl.glBindBufferBase(gl.GL_TRANSFORM_FEEDBACK_BUFFER, 0, buffer)
        gl.glEnable(gl.GL_RASTERIZER_DISCARD)
        near, far = 0.0625, 1024
        a = ctypes.c_float(far/(far-near)).value
        b = ctypes.c_float(-near*far/(far-near)).value
        for name, value in {'c[0]': (1,0,0,0), 'c[1]': (0,1,0,0),
                'c[2]': (0,0,a,b), 'c[3]': (0,0,1,0), 'c[58]': (1,1,1,0),
                'c[59]': (-0.5,-0.5,0,0), 'viewport_scale': (1,1,1,0)}.items():
            gl.glUniform4f(gl.glGetUniformLocation(program, name), *value)
        gl.glUniform1f(gl.glGetUniformLocation(program, 'point_size'), 1)
        for distance in (near, 1, 10, 100, 500, 1000):
            depths = []
            for z in (distance, distance+0.0016):
                gl.glVertexAttrib4f(0, 0, 0, z, 1)
                gl.glBeginTransformFeedback(gl.GL_POINTS)
                gl.glDrawArrays(gl.GL_POINTS, 0, 1)
                gl.glEndTransformFeedback()
                position = (ctypes.c_float*4)()
                gl.glGetBufferSubData(gl.GL_TRANSFORM_FEEDBACK_BUFFER, 0, 16, position)
                actual = position[2]/position[3]
                expected = ((1-a)*ctypes.c_float(z).value-b)/ctypes.c_float(z).value
                assert actual == pytest.approx(expected, rel=2e-6, abs=1e-12)
                depths.append(actual)
            assert depths[0] > depths[1], (distance, depths)
        assert gl.glGetError() == gl.GL_NO_ERROR
    finally:
        gl.glDisable(gl.GL_RASTERIZER_DISCARD)
        gl.glBindBufferBase(gl.GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0)
        gl.glDeleteBuffers(1, [buffer])
        gl.glDeleteVertexArrays(1, [vao])
        gl.glDeleteProgram(program)
        gl.glDeleteShader(shader)


@pytest.mark.parametrize('reversed_depth', [False, True])
@pytest.mark.parametrize('samples', [0, 4])
def test_gpu_fixed_bias_and_resolve(emitter, gpu, reversed_depth, samples):
    from OpenGL.GL.shaders import compileProgram, compileShader
    gl = gpu
    vertex = compileShader(emit(emitter, 'fixed', int(reversed_depth)), gl.GL_VERTEX_SHADER)
    fragment = compileShader('#version 450 core\nout vec4 color; void main(){color=vec4(1);}', gl.GL_FRAGMENT_SHADER)
    program = compileProgram(vertex, fragment)
    gl.glDeleteShader(vertex)
    gl.glDeleteShader(fragment)
    buffers, framebuffers = [], []
    vao = gl.glGenVertexArrays(1)
    vbo = gl.glGenBuffers(1)
    try:
        for count in (samples, 0):
            fbo = gl.glGenFramebuffers(1)
            framebuffers.append(fbo)
            gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, fbo)
            for attachment, format_ in ((gl.GL_COLOR_ATTACHMENT0, gl.GL_RGBA8),
                    (gl.GL_DEPTH_STENCIL_ATTACHMENT, gl.GL_DEPTH32F_STENCIL8 if reversed_depth else gl.GL_DEPTH24_STENCIL8)):
                renderbuffer = gl.glGenRenderbuffers(1)
                buffers.append(renderbuffer)
                gl.glBindRenderbuffer(gl.GL_RENDERBUFFER, renderbuffer)
                gl.glRenderbufferStorageMultisample(gl.GL_RENDERBUFFER, count, format_, 16, 16)
                gl.glFramebufferRenderbuffer(gl.GL_FRAMEBUFFER, attachment, gl.GL_RENDERBUFFER, renderbuffer)
            assert gl.glCheckFramebufferStatus(gl.GL_FRAMEBUFFER) == gl.GL_FRAMEBUFFER_COMPLETE
        gl.glClipControl(gl.GL_UPPER_LEFT, gl.GL_ZERO_TO_ONE)
        gl.glViewport(0, 0, 16, 16)
        gl.glEnable(gl.GL_DEPTH_TEST)
        gl.glDepthFunc(gl.GL_GREATER if reversed_depth else gl.GL_LESS)
        gl.glClearDepth(0 if reversed_depth else 1)
        gl.glUseProgram(program)
        gl.glBindVertexArray(vao)
        gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo)
        gl.glEnableVertexAttribArray(0)
        gl.glVertexAttribPointer(0, 4, gl.GL_FLOAT, False, 0, None)
        location = gl.glGetUniformLocation(program, 'depth_bias')
        for span in (1.0, 0.1):
            gl.glDepthRange(1-span if reversed_depth else 0, 1 if reversed_depth else span)
            for forward in (0.1, 0.9, 0.9999):
                vertices = (ctypes.c_float*12)(-1, -1, forward, 1, 3, -1, forward, 1, -1, 3, forward, 1)
                gl.glBufferData(gl.GL_ARRAY_BUFFER, ctypes.sizeof(vertices), vertices, gl.GL_STATIC_DRAW)
                # Draw with bias, then remove it. A stale uniform must not survive.
                for units in (8, 0):
                    gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, framebuffers[0])
                    gl.glClear(gl.GL_DEPTH_BUFFER_BIT | gl.GL_COLOR_BUFFER_BIT)
                    gl.glUniform1f(location, units/(16777215*span))
                    gl.glDrawArrays(gl.GL_TRIANGLES, 0, 3)
                    gl.glBindFramebuffer(gl.GL_READ_FRAMEBUFFER, framebuffers[0])
                    gl.glBindFramebuffer(gl.GL_DRAW_FRAMEBUFFER, framebuffers[1])
                    gl.glBlitFramebuffer(0, 0, 16, 16, 0, 0, 16, 16, gl.GL_DEPTH_BUFFER_BIT, gl.GL_NEAREST)
                    gl.glBindFramebuffer(gl.GL_READ_FRAMEBUFFER, framebuffers[1])
                    actual = ctypes.c_float()
                    gl.glReadPixels(8, 8, 1, 1, gl.GL_DEPTH_COMPONENT, gl.GL_FLOAT, ctypes.byref(actual))
                    stored_forward = ctypes.c_float(forward).value
                    expected = 1-stored_forward*span+units/16777215 if reversed_depth else stored_forward*span
                    assert actual.value == pytest.approx(expected, abs=1.2e-7)
                    assert gl.glGetError() == gl.GL_NO_ERROR
    finally:
        gl.glBindFramebuffer(gl.GL_FRAMEBUFFER, 0)
        gl.glDeleteFramebuffers(len(framebuffers), framebuffers)
        gl.glDeleteRenderbuffers(len(buffers), buffers)
        gl.glDeleteBuffers(1, [vbo])
        gl.glDeleteVertexArrays(1, [vao])
        gl.glDeleteProgram(program)
