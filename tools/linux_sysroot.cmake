# CMake toolchain of the portable Linux build's SDL3 (tools/linux_build.py):
# clang for 32-bit x86, against the system root of tools/linux_sysroot.py,
# given as -DHALO_SYSROOT=<folder>. The libraries SDL finds there it loads
# when it starts (dlopen), by the names it reads from them.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER_TARGET i686-linux-gnu)
set(CMAKE_SYSROOT "${HALO_SYSROOT}")
# CMake 4 no longer seeds the find paths from CMAKE_SYSROOT: without these,
# find_library looks only in /usr/local/lib and never in the system root's
# usr/lib/i386-linux-gnu, so every library the checks look for (SDL's X11)
# is missing and the configure fails.
set(CMAKE_FIND_ROOT_PATH "${HALO_SYSROOT}")
set(CMAKE_SYSTEM_PREFIX_PATH "/usr")
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH "/usr/lib/i386-linux-gnu")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "/usr/include")
set(CMAKE_LIBRARY_ARCHITECTURE i386-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${HALO_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR} "${HALO_SYSROOT}/usr/lib/i386-linux-gnu/pkgconfig:${HALO_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")
# (a program that a check links may use a library whose own libraries the
# system root has not got, as it has only what SDL's headers and dlopen
# names need: libX11's libxcb, in the check for XInput 2.1's scrolling)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--allow-shlib-undefined")
# (and in the projects CMake tries compiling in)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES HALO_SYSROOT)
