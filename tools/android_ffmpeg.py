"""FFmpeg for the Android guest: the movies' decoder.

The Android port runs the game as ILP32 AArch64 code (port/android/README.md):
native AArch64 instructions with 32-bit pointers, compiled by clang for
arm64_32-apple-watchos, its assembly converted to ELF (tools/android_asm_convert.py)
and assembled for aarch64-linux-android, with the port's own musl as its C
library. FFmpeg (the decoder behind the Bink SDK's entry points,
port/linux/src/bink_ffmpeg.c) is built for that same ABI here: the compiler
FFmpeg is given does the port's three steps per file, and the libraries this
leaves are what tools/android_build.py links into the guest image.

Only what the movies use is built: the Bink demuxer, its video and audio
decoders, swscale (the frame to the texture's BGRA) and swresample (the sound
to the mixer's 16-bit stereo). No assembly (FFmpeg's AArch64 is written for
64-bit pointers), no network, no programs, static, LGPL.

Usage: android_ffmpeg.py <build directory> [--ndk <ndk>]
"""

import hashlib
import os
import shutil
import subprocess
import sys
import tarfile
import urllib.request
from pathlib import Path

FFMPEG_VERSION = "7.1"
FFMPEG_URL = f"https://ffmpeg.org/releases/ffmpeg-{FFMPEG_VERSION}.tar.xz"
FFMPEG_SHA256 = "40973d44970dbc83ef302b0609f2e74982be2d85916dd2ee7472d30678a7abe6"
# (the release's own tarball: this is the digest of the file signed by the
# FFmpeg release signing key, B4322F04D67658D8)
MUSL_VERSION = "1.2.5"
# what the guest link takes (tools/android_build.py)
LIBRARIES = ["libavformat", "libavcodec", "libswscale", "libswresample", "libavutil"]

# the guest's ABI flags (tools/android_build.py's GUEST_ABI_FLAGS, what FFmpeg needs)
GUEST_FLAGS = ("--target=arm64_32-apple-watchos -U__APPLE__ -U__MACH__ "
               "-fno-define-target-os-macros -D__linux__=1 -D__unix__=1 "
               "-DHALO_ARM64_GUEST=1 -DHALO_ANDROID=1 -D_GNU_SOURCE -mcpu=cortex-a53 "
               "-nostdinc -fshort-wchar -fno-stack-protector -fno-unwind-tables "
               "-fno-asynchronous-unwind-tables -femulated-tls "
               "-mllvm -aarch64-neon-syntax=generic -ffp-contract=off -fno-pic "
               # (the AArch64 atomics inline: the outline helpers clang would
               # call are Android's, and their initialisation is bionic's)
               "-mno-outline-atomics")

# What the configure's tests answer (they run against Android's bionic) but the
# guest's musl has not: FFmpeg falls back to its own alternatives.
MUSL_MISSING = [
    # (the configure's tests run against Android's bionic, whose libc has more
    # than the guest's musl: what is missing is turned off and FFmpeg falls
    # back to what the guest does have)
    "HAVE_ARC4RANDOM_BUF", "HAVE_ARC4RANDOM", "HAVE_GETAUXVAL", "HAVE_ELF_AUX_INFO",
    "HAVE_SCHED_GETAFFINITY", "HAVE_SCHED_SETAFFINITY", "HAVE_PTHREAD_AFFINITY_NP",
    # (the guest's musl keeps lseek (its off_t is 64-bit) and has no mkstemp;
    # bionic has both, and its names would not link against the guest's libc)
    "HAVE_LSEEK64", "HAVE_MKSTEMP",
]


def find_ndk(given):
    """the Android NDK: the one given, ANDROID_NDK_HOME, or the newest under the SDK"""
    if given:
        return Path(given)
    from_env = os.environ.get("ANDROID_NDK_HOME")
    if from_env and Path(from_env).is_dir():
        return Path(from_env)
    for sdk in (Path.home() / "Android" / "Sdk", Path(os.environ.get("ANDROID_HOME", "/nonexistent"))):
        if (sdk / "ndk").is_dir():
            versions = sorted((sdk / "ndk").iterdir())
            if versions:
                return versions[-1]
    raise SystemExit("android_ffmpeg.py: no NDK (set ANDROID_NDK_HOME or pass --ndk)")


def run(command, **kwargs):
    subprocess.run([str(part) for part in command], check=True, **kwargs)


def fetch(root):
    """the FFmpeg source, once"""
    source = root / "src"
    if (source / "configure").is_file():
        return source
    archive = root / f"ffmpeg-{FFMPEG_VERSION}.tar.xz"
    print(f"{Path(__file__).name}: downloading FFmpeg {FFMPEG_VERSION}")
    urllib.request.urlretrieve(FFMPEG_URL, archive)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != FFMPEG_SHA256:
        archive.unlink()
        raise SystemExit(f"{Path(__file__).name}: {archive.name}: SHA-256 {digest}, "
                         f"expected {FFMPEG_SHA256}")
    with tarfile.open(archive) as tar:
        tar.extractall(root)
    (root / f"ffmpeg-{FFMPEG_VERSION}").rename(source)
    archive.unlink()
    return source


def write_guest_compiler(root, tree, ndk):
    """FFmpeg's compiler: one C file becomes the guest's object through the same
    three steps the port's own sources take (its assembly converter and the
    guest's musl are what the flags point at)."""
    toolchain = ndk / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64"
    clang = toolchain / "bin" / "clang"
    resource = subprocess.check_output([str(clang), "-print-resource-dir"], text=True).strip()
    musl = tree / "build" / "android" / "third_party" / f"musl-{MUSL_VERSION}"
    libc_include = tree / "build" / "android" / "guest" / "libc_include"
    arch = tree / "port" / "android" / "guest" / "libc" / "arch" / "arm64_32"
    convert = tree / "tools" / "android_asm_convert.py"
    # (the guest's own <sys/cdefs.h>: clang's headers want it, musl has none)
    compat = tree / "port" / "android" / "guest" / "compat" / "include"
    wrapper = root / "guest-cc.sh"
    wrapper.write_text(f"""#!/bin/bash
# android_ffmpeg.py wrote this: the compiler FFmpeg builds the guest with.
set -e
CLANG="{clang}"
args=("$@")
compile=0
src=""
out=""
depfile=""
filtered=()
for ((i = 0; i < ${{#args[@]}}; i++)); do
    arg="${{args[i]}}"
    case "$arg" in
        -c) compile=1 ;;
        -o) out="${{args[i+1]}}"; i=$((i+1)) ;;
        -MF) depfile="${{args[i+1]}}"; i=$((i+1)) ;;
        -MT) i=$((i+1)) ;;
        -march=*|-mcpu=*|-mtune=*|-mfpu=*|-fPIC|-fpic|-fno-pic|-fexceptions|-funwind-tables) ;;
        *.c) src="$arg" ;;
        *) filtered+=("$arg") ;;
    esac
done
if [ "$compile" != 1 ] || [ -z "$src" ] || [ -z "$out" ]; then
    # (nothing in this build compiles without -c: the configure has its own)
    exec "$CLANG" "$@"
fi
"$CLANG" "${{filtered[@]}}" {GUEST_FLAGS} \\
    -isystem "{libc_include}" -isystem "{arch}" -isystem "{musl}/arch/generic" \\
    -isystem "{musl}/include" -isystem "{resource}/include" -idirafter "{compat}" \\
    -S "$src" -o "$out.darwin.s"
python3 "{convert}" "$out.darwin.s" "$out.s"
"$CLANG" --target=aarch64-linux-android -c "$out.s" -o "$out"
if [ -n "$depfile" ]; then printf '%s: %s\\n' "$out" "$src" > "$depfile"; fi
""")
    wrapper.chmod(0o755)
    return wrapper, toolchain


def patch_config(root):
    """the features the configure found in bionic but the guest's musl lacks"""
    path = root / "build" / "config.h"
    text = path.read_text()
    off = []
    for name in MUSL_MISSING:
        replaced = text.replace(f"#define {name} 1", f"#define {name} 0")
        if replaced != text:
            off.append(name)
            text = replaced
    path.write_text(text)
    print(f"android_ffmpeg.py: off for musl: {', '.join(off) if off else '(none)'}")


def main():
    arguments = [argument for argument in sys.argv[1:] if not argument.startswith("--")]
    ndk_argument = sys.argv[sys.argv.index("--ndk") + 1] if "--ndk" in sys.argv else None
    if not arguments:
        raise SystemExit(__doc__)
    root = Path(arguments[0]).resolve()
    tree = Path.cwd()
    root.mkdir(parents=True, exist_ok=True)
    ndk = find_ndk(ndk_argument)
    toolchain = ndk / "toolchains" / "llvm" / "prebuilt" / "linux-x86_64"
    print(f"android_ffmpeg.py: NDK {ndk}")

    source = fetch(root)
    print(f"android_ffmpeg.py: FFmpeg {FFMPEG_VERSION} in {source}")

    build = root / "build"
    configure = [source / "configure",
                 f"--prefix={root / 'out'}",
                 # (the guest's C library is musl: a Linux libc, not Android's
                 # bionic, and its headers and names are what has to be built
                 # against; the movies decode synchronously, so no threads)
                 "--target-os=linux", "--arch=aarch64", "--disable-pthreads",
                 "--enable-cross-compile", f"--cc={toolchain / 'bin' / 'clang'}",
                 "--extra-cflags=--target=aarch64-linux-android28",
                 "--extra-ldflags=--target=aarch64-linux-android28",
                 "--disable-everything", "--disable-programs", "--disable-doc",
                 "--disable-network", "--disable-autodetect", "--disable-asm",
                 "--disable-shared", "--enable-static", "--disable-debug", "--enable-small",
                 "--disable-avfilter", "--disable-avdevice", "--disable-postproc",
                 # (its path is glibc's <asm/unistd.h>: perf profiling only)
                 "--disable-linux-perf",
                 "--enable-demuxer=bink", # (the video decoder's configure name is "bink": "binkvideo" is only its
                 # name once running)
                 "--enable-decoder=bink,binkaudio_dct,binkaudio_rdft",
                 "--enable-protocol=file", "--enable-swscale", "--enable-swresample"]
    stamp = root / "configure.stamp"
    if stamp.is_file() and stamp.read_text() != "\n".join(str(part) for part in configure):
        # (the options changed: the whole build starts over, generated sources
        # such as libavcodec/codec_list.c among them)
        shutil.rmtree(build)
    if not (build / "config.h").is_file():
        build.mkdir(parents=True, exist_ok=True)
        with open(root / "configure.log", "w") as log:
            run(configure, cwd=build, stdout=log, stderr=subprocess.STDOUT)
        stamp.write_text("\n".join(str(part) for part in configure))
    patch_config(root)

    wrapper, toolchain = write_guest_compiler(root, tree, ndk)
    # (the compiler is part of what the objects are: a change to it has to
    # rebuild them, which make alone does not know)
    compiler_stamp = root / "compiler.stamp"
    compiler = wrapper.read_text() + "\n".join(GUEST_FLAGS)
    if not compiler_stamp.is_file() or compiler_stamp.read_text() != compiler:
        print("android_ffmpeg.py: the compiler changed, rebuilding from scratch")
        run(["make", "clean"], cwd=build)
    compiler_stamp.write_text(compiler)
    print("android_ffmpeg.py: building with the guest's compiler")
    with open(root / "build.log", "w") as log:
        try:
            run(["make", "-j", str(os.cpu_count() or 4), f"CC={wrapper}",
                 f"AR={toolchain / 'bin' / 'llvm-ar'}", f"RANLIB={toolchain / 'bin' / 'llvm-ranlib'}"],
                cwd=build, stdout=log, stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError:
            # (the compiler's own words are the only clue to a failure, and this
            # log is their only home: show the tail before giving up)
            print(f"android_ffmpeg.py: make failed, the tail of {root / 'build.log'}:")
            print("\n".join((root / "build.log").read_text(errors="replace").splitlines()[-40:]))
            raise

    # FFmpeg's configure answered some of its tests against Android's bionic
    # (the configure has to run a working compiler): its objects call errno and
    # lseek64 by bionic's names, and the AArch64 builtins clang links ask
    # bionic for the CPU properties. The guest's musl has none of those names:
    # errno is musl's own accessor, lseek is the same call on this ABI, and the
    # properties are reported absent (nothing the movies need), so the three
    # stand in and the guest links.
    compat_source = root / "guest-compat.c"
    compat_source.write_text(
        "#include <errno.h>\n"
        "#include <unistd.h>\n"
        "\n"
        "int *__errno(void) { return __errno_location(); }\n"
        "off_t lseek64(int descriptor, off_t offset, int whence)\n"
        "{\n"
        "    return lseek(descriptor, offset, whence);\n"
        "}\n"
        "int __system_property_get(const char *name, char *value)\n"
        "{\n"
        "    (void)name;\n"
        "    if (value)\n"
        "        value[0] = 0;\n"
        "    return 0;\n"
        "}\n")
    compat_object = root / "guest-compat.o"
    run([wrapper, "-c", compat_source, "-o", compat_object])
    run([toolchain / "bin" / "llvm-ar", "rcs", build / "libavutil" / "libavutil.a", compat_object])
    print("android_ffmpeg.py: guest compat (errno, lseek64, properties)")

    missing = [library for library in LIBRARIES if not (build / library / f"{library}.a").is_file()]
    if missing:
        raise SystemExit("android_ffmpeg.py: missing " + ", ".join(missing))
    # (the stamp tools/android_build.py waits for: the headers exist from here,
    # whatever the build's order)
    # (the guest objects take this stamp as an input: write it only when
    # its content changes, so that a rebuild of these libraries does not
    # recompile the whole guest)
    stamp = root / "ffmpeg.stamp"
    content = "\n".join(str(part) for part in LIBRARIES) + "\n"
    if not stamp.is_file() or stamp.read_text() != content:
        stamp.write_text(content)
    print("android_ffmpeg.py: " + ", ".join(f"{library}.a" for library in LIBRARIES))


if __name__ == "__main__":
    main()
