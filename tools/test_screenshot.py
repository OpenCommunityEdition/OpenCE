"""Check rebindable screenshot actions and real PNG readback in a hidden GL context."""
import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def block(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


PRELUDE = r'''
#include <SDL3/SDL.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gl.h"
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s (%s)\n", __LINE__, #c, SDL_GetError()); exit(1); } } while (0)
#include "../include/halo_keyboard.h"
#undef INPUT_MOUSE
#include "sdl_platform.h"
#define GL_DEFINE(name) __typeof__(name) name;
GL_FUNCTIONS(GL_DEFINE)
#undef GL_DEFINE
struct render_target_entry { struct {unsigned long gl_width, gl_height; GLuint texture;} target; };
static GLuint capture_framebuffer;
static char data_root[512], message[256];
static BOOL fail_save, screenshot_requested;
static int input_lock, locks;
static Uint64 wheel_press_until_ms;
static int wheel_direction;
static unsigned long config_revision;
static const char *screenshot_binding = "F10", *fire_binding = "Mouse Left";
static unsigned long config_changes(void) {return config_revision;}
static const char *config_string(const char *name) {
    if (!strcmp(name, "controls.screenshot")) return screenshot_binding;
    if (!strcmp(name, "controls.fire")) return fire_binding;
    if (!strcmp(name, "controls.move_forward")) return "W";
    if (!strcmp(name, "controls.pause")) return "Escape";
    if (!strcmp(name, "controls.scoreboard")) return "Tab";
    return "";
}
static void platform_log(const char *format, ...) {(void)format; CHECK(FALSE);}
typedef struct {unsigned short wButtons;} XINPUT_GAMEPAD;
#define XINPUT_GAMEPAD_START 0x10
#define XINPUT_GAMEPAD_BACK 0x20
enum {_binding_capture_waiting, _binding_capture_taken};
static int binding_capture, binding_capture_result, binding_captured_input;
static Uint64 binding_taken_ms;
static void pthread_mutex_lock(int *lock) { (void)lock; CHECK(!locks); locks++; }
static void pthread_mutex_unlock(int *lock) { (void)lock; CHECK(locks == 1); locks--; }
static const char *platform_data_root(void) {return data_root;}
static GLuint framebuffer_get(GLuint color, GLuint depth) {
    (void)color; (void)depth;
    /* A cache miss binds both framebuffer targets in the real renderer. */
    glBindFramebuffer(GL_FRAMEBUFFER, capture_framebuffer);
    return capture_framebuffer;
}
static void console_printf(unsigned char clear, const char *format, ...) {
    va_list args; (void)clear;
    va_start(args, format); vsnprintf(message, sizeof(message), format, args); va_end(args);
}
static bool fixed_current_time(SDL_Time *time) {*time = 0; return true;}
static bool fixed_date(SDL_Time time, SDL_DateTime *date, bool local) {
    (void)time; CHECK(local);
    *date = (SDL_DateTime){.year=2026, .month=8, .day=25, .hour=14, .minute=3, .second=27};
    return true;
}
static bool save_png(SDL_Surface *surface, const char *path) {
    if (fail_save) return SDL_SetError("Simulated disk write failure");
    return SDL_SavePNG(surface, path);
}
#define SDL_GetCurrentTime fixed_current_time
#define SDL_TimeToDateTime fixed_date
#define SDL_SavePNG save_png
'''

TESTS = r'''
static BOOL poll_screenshot(const struct platform_input_state *input) {
    keyboard_screenshot(keyboard_bound_actions(input));
    return platform_screenshot_take_request();
}
static void check_bindings(void) {
    struct platform_input_state input = {0};
    XINPUT_GAMEPAD pad = {0};
    SDL_Event event = {0};
    input.keys[SDL_SCANCODE_F2] = 1; CHECK(!poll_screenshot(&input));
    input.keys[SDL_SCANCODE_F10] = 1; CHECK(poll_screenshot(&input));
    CHECK(!poll_screenshot(&input) && !platform_screenshot_take_request() && !locks);
    input.keys[SDL_SCANCODE_F10] = 0; CHECK(!poll_screenshot(&input));
    input.keys[SDL_SCANCODE_F10] = 1; CHECK(poll_screenshot(&input));
    /* Rebinding removes the old default; the two slots use the normal parser. */
    screenshot_binding = "F2, Mouse 4"; config_revision++;
    input.keys[SDL_SCANCODE_F2] = 0; CHECK(!poll_screenshot(&input));
    input.keys[SDL_SCANCODE_F2] = 1; CHECK(poll_screenshot(&input));
    input.keys[SDL_SCANCODE_F2] = 0; CHECK(!poll_screenshot(&input));
    input.mouse_buttons[SDL_BUTTON_X1] = 1; CHECK(poll_screenshot(&input));
    CHECK(!poll_screenshot(&input));
    input.mouse_buttons[SDL_BUTTON_X1] = 0; CHECK(!poll_screenshot(&input));
    /* The default is free for other actions. No hidden screenshot protection. */
    fire_binding = "F10"; config_revision++;
    CHECK(keyboard_bound_actions(&input) & (1UL << HALO_KEYBOARD_FIRE));
    CHECK(!poll_screenshot(&input));
    screenshot_binding = ""; config_revision++;
    input.keys[SDL_SCANCODE_F2] = 1; CHECK(!poll_screenshot(&input));
    screenshot_binding = "Wheel Down"; config_revision++;
    wheel_direction = -1; wheel_press_until_ms = SDL_GetTicks() + 1000;
    CHECK(poll_screenshot(&input)); CHECK(!poll_screenshot(&input));
    wheel_press_until_ms = 0; CHECK(!poll_screenshot(&input));
    screenshot_binding = "F10"; config_revision++;
    input.menus = TRUE; CHECK(poll_screenshot(&input));
    input.keys[SDL_SCANCODE_ESCAPE] = input.keys[SDL_SCANCODE_TAB] = input.keys[SDL_SCANCODE_W] = 1;
    unsigned long held = keyboard_bound_actions(&input);
    keyboard_controls(held, &pad);
    CHECK(pad.wButtons == (XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_BACK));
    CHECK(keyboard_actions_held == held && (held & (1UL << HALO_KEYBOARD_MOVE_FORWARD)));
    keyboard_screenshot(0); CHECK(!platform_screenshot_take_request());
    /* Normal binding capture accepts both F2 and F10, with no capture fired. */
    event.key.down = true;
    for (int i = 0; i < 2; i++) {
        binding_capture = _binding_capture_waiting;
        event.key.scancode = i ? SDL_SCANCODE_F10 : SDL_SCANCODE_F2;
        capture_key(event);
        CHECK(binding_capture == _binding_capture_taken && binding_capture_result == 1);
        CHECK(binding_captured_input == event.key.scancode && !platform_screenshot_take_request());
    }
    binding_capture = _binding_capture_waiting;
    event.key.repeat = true; capture_key(event);
    CHECK(binding_capture == _binding_capture_waiting);
}
static void check_state(GLuint pack_buffer) {
    GLint value;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &value); CHECK(value == 0);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value); CHECK(value == 0);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &value); CHECK(value == (GLint)pack_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &value); CHECK(value == 8);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &value); CHECK(value == 8);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &value); CHECK(value == 1);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &value); CHECK(value == 2);
    CHECK(glGetError() == GL_NO_ERROR);
}
static void check_png(const char *filename, const Uint8 *colors) {
    char path[640]; Uint8 r, g, b, a;
    snprintf(path, sizeof(path), "%s/screenshots/%s", data_root, filename);
    SDL_Surface *image = SDL_LoadPNG(path);
    CHECK(image && image->w == 3 && image->h == 2);
    for (int y = 0; y < 2; y++) for (int x = 0; x < 3; x++) {
        const Uint8 *expected = colors + (y * 3 + x) * 4;
        CHECK(SDL_ReadSurfacePixel(image, x, y, &r, &g, &b, &a));
        CHECK(r == expected[0] && g == expected[1] && b == expected[2] && a == 255);
    }
    SDL_DestroySurface(image);
}
int main(int argc, char **argv) {
    CHECK(argc == 2); snprintf(data_root, sizeof(data_root), "%s", argv[1]);
    check_bindings();

    CHECK(SDL_Init(SDL_INIT_VIDEO));
    CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4));
    CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5));
    CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE));
    SDL_Window *window = SDL_CreateWindow("Screenshot regression", 32, 32, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    CHECK(window);
    SDL_GLContext context = SDL_GL_CreateContext(window); CHECK(context);
#define HALO_TEST_LOAD_GL(name) name = (__typeof__(name))SDL_GL_GetProcAddress(#name); CHECK(name);
    GL_FUNCTIONS(HALO_TEST_LOAD_GL)
#undef HALO_TEST_LOAD_GL
    struct render_target_entry target = {.target = {.gl_width = 3, .gl_height = 2}};
    const Uint8 colors[24] = {255,0,0,1, 0,255,0,2, 0,0,255,3, 255,255,255,4, 13,29,47,5, 127,63,31,6};
    glGenTextures(1, &target.target.texture); glBindTexture(GL_TEXTURE_2D, target.target.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 3, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, colors);
    glGenFramebuffers(1, &capture_framebuffer); glBindFramebuffer(GL_FRAMEBUFFER, capture_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.target.texture, 0);
    CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLuint pack_buffer; glGenBuffers(1, &pack_buffer); glBindBuffer(GL_PIXEL_PACK_BUFFER, pack_buffer);
    glBufferData(GL_PIXEL_PACK_BUFFER, 32, NULL, GL_STREAM_READ);
    glPixelStorei(GL_PACK_ALIGNMENT, 8); glPixelStorei(GL_PACK_ROW_LENGTH, 8);
    glPixelStorei(GL_PACK_SKIP_ROWS, 1); glPixelStorei(GL_PACK_SKIP_PIXELS, 2);
    CHECK(glGetError() == GL_NO_ERROR);
    write_key_screenshot(&target);
    CHECK(!strcmp(message, "Saved screenshot as 2026-08-25_14.03.27.png"));
    check_state(pack_buffer); check_png("2026-08-25_14.03.27.png", colors);
    write_key_screenshot(&target);
    CHECK(!strcmp(message, "Saved screenshot as 2026-08-25_14.03.27_2.png"));
    check_state(pack_buffer); check_png("2026-08-25_14.03.27_2.png", colors);
    check_png("2026-08-25_14.03.27.png", colors);
    fail_save = TRUE; write_key_screenshot(&target);
    CHECK(!strncmp(message, "Screenshot failed:", 18)); check_state(pack_buffer);
    fail_save = FALSE;
    target.target.gl_width = 0; write_key_screenshot(&target);
    CHECK(!strncmp(message, "Screenshot failed:", 18)); check_state(pack_buffer);
    glDeleteBuffers(1, &pack_buffer); glDeleteFramebuffers(1, &capture_framebuffer);
    glDeleteTextures(1, &target.target.texture);
    SDL_GL_DestroyContext(context); SDL_DestroyWindow(window); SDL_Quit();
    puts("PASS: F10 default; normal rebind/clear/two slots/mouse/wheel; F2/F10 binding capture; gameplay unchanged; real GL-to-PNG pixels, orientation/opaque alpha; timestamps/collisions; GL state and failures");
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    args = parser.parse_args()
    render = (ROOT / 'port/linux/src/d3d8_gl.c').read_text()
    events = (ROOT / 'port/linux/src/sdl_platform.c').read_text()
    keyboard = (ROOT / 'port/linux/src/xinput_sdl.c').read_text()
    menu = (ROOT / 'port/linux/game/menu_functions.c').read_text()
    config = (ROOT / 'port/linux/src/port_config.c').read_text()
    assert '{ "controls.pause", L"PAUSE MENU", 2 },\n\t{ "controls.screenshot", L"SCREENSHOT", 2 },' in menu
    assert r'"controls.screenshot", _config_string, "\"F10\""' in config
    assert 'SDL_SCANCODE_F2' not in events and 'SDL_SCANCODE_F10' not in events
    unit = (PRELUDE + block(events, 'void platform_screenshot_request(void)') +
            block(events, 'BOOL platform_screenshot_take_request(void)') +
            '\n' + keyboard[keyboard.index('#define MAXIMUM_BINDINGS'):keyboard.index('unsigned long halo_keyboard_actions(')] +
            '\nstatic void capture_key(SDL_Event event) { do {\n' +
            block(events, 'if (binding_capture == _binding_capture_waiting && event.key.down') + '\n} while (0); }\n' +
            block(render, 'static void write_key_screenshot(') + TESTS)
    compiler = [args.cc, '-std=gnu11', '-O2', '-fuse-ld=lld', '-I' + str(ROOT / 'port/linux/src')]
    env = dict(os.environ)
    if sys.platform == 'win32':
        sdl = next((ROOT / 'build/windows/third_party').glob('SDL3-*/include')).parent
        compiler += ['--target=i686-pc-windows-msvc', '-I' + str(sdl / 'include')]
        libraries = [str(sdl / 'lib/x86/SDL3.lib')]
        env['PATH'] = str(sdl / 'lib/x86') + os.pathsep + env['PATH']
    else:
        compiler += subprocess.check_output(['pkg-config', '--cflags', 'sdl3'], text=True).split()
        libraries = subprocess.check_output(['pkg-config', '--libs', 'sdl3'], text=True).split()
    with tempfile.TemporaryDirectory(prefix='halo-screenshot-test-') as directory:
        path = Path(directory)
        (path / 'screenshot.c').write_text(unit)
        subprocess.run([*compiler, str(path / 'screenshot.c'), *libraries, '-o', str(path / 'screenshot.exe')], check=True)
        subprocess.run([str(path / 'screenshot.exe'), str(path)], env=env, check=True, timeout=30)
        files = sorted((path / 'screenshots').glob('*.png'))
        assert len(files) == 2
        for file in files:
            assert file.read_bytes().startswith(b'\x89PNG\r\n\x1a\n')


if __name__ == '__main__':
    main()
