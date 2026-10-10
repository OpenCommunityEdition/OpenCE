# Copyright 2025 The Emscripten Authors. All rights reserved.
# Emscripten is available under the MIT and University of Illinois/NCSA
# licenses. See the Emscripten LICENSE file for details.

"""Pinned SDL3 Emscripten port for Halo's threaded browser build.

SDL 3.4.2 reads ``navigator.getGamepads`` directly from a Web Worker while
enumerating joystick metadata. Browsers do not expose that API in workers, so
connecting a controller aborts the pthread build before Halo can start. SDL
3.4.16 proxies those reads to the browser main thread.

Keep this as a user port instead of modifying the downloaded Emscripten SDK so
local builds and CI consume the same fixed SDL release.
"""

import glob
import os
import shutil


VERSION = "3.4.16"
TAG = f"release-{VERSION}"
HASH = (
    "af4109305de11e94619404feffd2874f26a25cbde71b7c8d8f3cd2210232858f1"
    "8061df85df6a4467d5109f168381d6eb4dc1e27bd375d12f7448d1cbd71da41"
)
SUBDIR = f"SDL-{TAG}"
PROJECT = "halo_sdl3"


def get_lib_name(settings):
    return "libhalo-SDL3" + ("-mt" if settings.PTHREADS else "") + ".a"


def get(ports, settings, shared):
    ports.fetch_project(
        PROJECT,
        f"https://github.com/libsdl-org/SDL/archive/{TAG}.zip",
        sha512hash=HASH,
    )

    def create(final):
        root_dir = ports.get_dir(PROJECT, SUBDIR)

        # Emscripten ships a generated platform configuration that its SDL
        # port uses in place of running CMake for every build.
        build_config = shared.path_from_root(
            "tools", "ports", "sdl3", "SDL_build_config.h"
        )
        shutil.copyfile(
            build_config,
            os.path.join(root_dir, "include", "SDL3", "SDL_build_config.h"),
        )

        source_include_path = os.path.join(root_dir, "include", "SDL3")
        ports.install_headers(source_include_path, target="SDL3")

        glob_patterns = [
            # Generic sources from SDL's CMakeLists.txt.
            "*.c",
            "atomic/*.c",
            "audio/*.c",
            "camera/*.c",
            "core/unix/*.c",
            "cpuinfo/*.c",
            "dynapi/*.c",
            "events/*.c",
            "io/*.c",
            "io/generic/*.c",
            "filesystem/*.c",
            "gpu/*.c",
            "joystick/*.c",
            "haptic/*.c",
            "hidapi/*.c",
            "locale/*.c",
            "main/*.c",
            "misc/*.c",
            "power/*.c",
            "render/*.c",
            "render/*/*.c",
            "sensor/*.c",
            "stdlib/*.c",
            "storage/*.c",
            "thread/*.c",
            "time/*.c",
            "timer/*.c",
            "video/*.c",
            "video/yuv2rgb/*.c",
            "tray/*.c",
            # Emscripten and fallback backends.
            "storage/generic/*.c",
            "tray/unix/*.c",
            "time/unix/*.c",
            "timer/unix/*.c",
            "main/emscripten/*.c",
            "filesystem/posix/*.c",
            "filesystem/emscripten/*.c",
            "locale/emscripten/*.c",
            "camera/emscripten/*.c",
            "joystick/emscripten/*.c",
            "joystick/virtual/*.c",
            "power/emscripten/*.c",
            "misc/emscripten/*.c",
            "audio/emscripten/*.c",
            "video/emscripten/*.c",
            "video/offscreen/*.c",
            "audio/disk/*.c",
            "loadso/dlopen/*.c",
            "camera/dummy/*.c",
            "audio/dummy/*.c",
            "video/dummy/*.c",
            "haptic/dummy/*.c",
            "sensor/dummy/*.c",
        ]

        flags = []
        if settings.PTHREADS:
            glob_patterns.append("thread/pthread/*.c")
            flags.append("-pthread")
        else:
            glob_patterns.append("thread/generic/*.c")

        sources = []
        for pattern in glob_patterns:
            matches = glob.glob(os.path.join(root_dir, "src", pattern))
            assert matches, f"SDL source pattern did not match: {pattern}"
            sources.extend(matches)

        includes = [ports.get_include_dir("SDL3"), os.path.join(root_dir, "src")]
        ports.build_port(
            root_dir,
            final,
            "halo_sdl3",
            srcs=sources,
            includes=includes,
            flags=flags,
        )

    return [shared.cache.get_lib(get_lib_name(settings), create, what="port")]


def clear(ports, settings, shared):
    shared.cache.erase_lib(get_lib_name(settings))


def show():
    return f"Halo SDL {VERSION} browser port (zlib license)"
