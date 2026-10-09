# CMake toolchain of the portable Linux build's SDL3 (tools/linux_build.py):
# clang for 32-bit x86, against the system root of tools/linux_sysroot.py,
# given as -DHALO_SYSROOT=<folder>. The libraries SDL finds there it loads
# when it starts (dlopen), by the names it reads from them.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER_TARGET i686-linux-gnu)
set(CMAKE_SYSROOT "${HALO_SYSROOT}")
set(CMAKE_LIBRARY_ARCHITECTURE i386-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${HALO_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR} "${HALO_SYSROOT}/usr/lib/i386-linux-gnu/pkgconfig:${HALO_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")
# (and in the projects CMake tries compiling in)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES HALO_SYSROOT)
