# tools/linux_armv7.cmake: SDL3 for the armv7 build. CMake toolchain of the
# portable Linux armv7 build's SDL3: clang for 32-bit ARM (AArch32: ARMv7-A,
# armhf, hard float), against an armhf system root, given as -DHALO_SYSROOT=<folder>. The libraries
# SDL finds there it loads when it starts (dlopen), by the names it reads
# from them.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER_TARGET arm-linux-gnueabihf)
set(CMAKE_C_FLAGS_INIT "-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard")
set(CMAKE_SYSROOT "${HALO_SYSROOT}")
# CMake 4 no longer seeds the find paths from CMAKE_SYSROOT: without these,
# find_library looks only in /usr/local/lib and never in the system root's
# usr/lib/arm-linux-gnueabihf, so every library the checks look for (SDL's X11)
# is missing and the configure fails.
set(CMAKE_FIND_ROOT_PATH "${HALO_SYSROOT}")
set(CMAKE_SYSTEM_PREFIX_PATH "/usr")
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH "/usr/lib/arm-linux-gnueabihf")
list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "/usr/include")
set(CMAKE_LIBRARY_ARCHITECTURE arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${HALO_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR} "${HALO_SYSROOT}/usr/lib/arm-linux-gnueabihf/pkgconfig:${HALO_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")
# (a program that a check links may use a library whose own libraries the
# system root has not got, as it has only what SDL's headers and dlopen
# names need: libX11's libxcb, in the check for XInput 2.1's scrolling)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld -Wl,--allow-shlib-undefined")
# (and in the projects CMake tries compiling in)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES HALO_SYSROOT)
# (the host's GNU ld cannot link 32-bit ARM: lld links every kind of target)
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld -Wl,--allow-shlib-undefined")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
