#!/bin/bash
# Build PowerSpark (the Mac build: PowerPC and Intel, Mac OS X 10.4 and later) on a Linux host, with Apple's gcc 4.2
# built as cross compilers (osxcross's ppc-test branch, cctools/ld64) and the Mac OS X SDKs copied from a Mac.
# The Apple SDKs and the toolchain stay on the build host (MACCROSS_ROOT, default ~/opt/maccross); nothing of
# them is in the repository.
#
# usage: tools/mac_cross_build.sh COMMAND [arguments]
#   toolchain          build the cross compilers into MACCROSS_ROOT (needs the SDKs in MACCROSS_ROOT/sdks, see below)
#   setup              write the -arch compiler driver (bin/maccc, also gcc-4.0, gcc-4.2 and cc) and the tool links
#   session [OUT]      the rendered session, tools/mac_rendered_session_checks.sh built only, one process per
#                      architecture (default OUT build/mac-cross-session): OUT/ppc/ and OUT/i386/ hold
#                      mac_rendered_session_ppc and mac_rendered_session_i386
#   controller         OpenCE Controller Setup.app (port/mac/controller_setup/build.sh), 10.5 SDK, in that directory
#   app [OUT]          PowerSpark.app without the dmg, from OUT/ppc and OUT/i386 of `session` (hdiutil is a Mac's)
#
# The host needs gcc, make, python3, rsync and, for the toolchain, clang, wget, patch. The session links objects built
# by the Macs' gates (build/mac-game-tick, mac-audit, mac-graphics-audit, mac-game-tick-session-final*); copy them
# from the Mac checkout's build folder, which the manifest names:
#   for d in $(sed 's#/[^/]*$##' build/mac-game-tick-session-final38/engine_objects_*.txt | sort -u); do
#       rsync -aL MAC:CHECKOUT/$d/ $d/; done
# The SDKs: rsync -a MAC:/Developer/SDKs/MacOSX10.4u.sdk/ MACCROSS_ROOT/sdks/MacOSX10.4u.sdk/ (also MacOSX10.5.sdk,
# for Controller Setup). Not tar: tar over a pipe from a Mac turns the SDKs' symbolic links into empty files.
#
# The scripts the Macs use run unchanged: the driver takes gcc-4.0 and gcc-4.2's place, accepts several -arch (one
# compiler each, joined with lipo) and turns -isysroot /Developer/SDKs/X into MACCROSS_ROOT/sdks/X. The host cannot
# run Mach-O programs, so the tests those scripts run on the Mac are compiled but not run (MAC_CROSS=1); the session
# is built only (MAC_RENDERED_SESSION_BUILD_ONLY=1), and run on a Mac.
set -eu

here=$(CDPATH= cd "$(dirname "$0")" && pwd)
repo=$(dirname "$here")
MACCROSS_ROOT=${MACCROSS_ROOT:-$HOME/opt/maccross}
export MACCROSS_ROOT
cmd=${1:-}
[ $# -eq 0 ] || shift

setup_env() {
    # the semantics header's stamp (tools/mac_audit_bodies.sh) hashes the headers in `sort` order: the Macs sort as C does
    LC_ALL=C
    export LC_ALL
    PATH=$MACCROSS_ROOT/bin:$MACCROSS_ROOT/host/bin:$PATH
    export PATH
    if [ ! -x "$MACCROSS_ROOT/bin/maccc" ]; then
        echo "mac_cross_build: no toolchain in $MACCROSS_ROOT (run: $0 toolchain; then $0 setup)" >&2
        exit 2
    fi
}

write_driver() {
    mkdir -p "$MACCROSS_ROOT/bin" "$MACCROSS_ROOT/shim"
    cat > "$MACCROSS_ROOT/bin/maccc" <<'DRIVER'
#!/bin/bash
# Apple gcc's -arch driver on the Linux cross compilers (tools/mac_cross_build.sh writes this file).
# One -arch runs that architecture's compiler; several run each (together) and join the results with lipo.
set -e
here=$(cd "$(dirname "$0")" && pwd)
root=${MACCROSS_ROOT:-$(dirname "$here")}
tbin=$root/target/bin
sysroot=; archs=(); args=(); out=; compile_only=0; i=0
argv=("$@")
n=${#argv[@]}
while [ $i -lt $n ]; do
    a=${argv[$i]}
    case "$a" in
        -arch) archs+=("${argv[$((i+1))]}"); i=$((i+2)); continue ;;
        -o) out=${argv[$((i+1))]}; args+=("$a" "$out"); i=$((i+2)); continue ;;
        -isysroot)
            sysroot=${argv[$((i+1))]}
            case "$sysroot" in /Developer/SDKs/*) sysroot=$root/sdks/${sysroot#/Developer/SDKs/} ;; esac
            args+=("$a" "$sysroot"); i=$((i+2)); continue ;;
        -o?*) out=${a#-o}; args+=("$a") ;;
        -c|-S|-E|-M|-MM|-fsyntax-only) compile_only=1; args+=("$a") ;;
        *) args+=("$a") ;;
    esac
    i=$((i+1))
done
[ ${#archs[@]} -gt 0 ] || archs=(i386)
# the compiler's own headers (stdarg.h, stddef.h) are not on its search path with a sysroot; the shim directory
# stands in for the Mac's case-insensitive file system (the game includes <StdDef.h>)
gccinc() {
    echo "-isystem $root/shim"
    [ -z "$sysroot" ] || echo "--sysroot=$sysroot"
    case "$1" in
        ppc) echo "-isystem $root/target/lib/gcc/powerpc-apple-darwin8/4.2.1/include" ;;
        i386) echo "-isystem $root/target/lib/gcc/i386-apple-darwin8/4.2.1/include" ;;
    esac
}
cc_for() {
    case "$1" in
        ppc) echo "$tbin/powerpc-apple-darwin8-gcc" ;;
        i386) echo "$tbin/i386-apple-darwin8-gcc" ;;
        *) echo "maccc: unsupported -arch $1" >&2; exit 1 ;;
    esac
}
# the compiler finds its own tools (ld, as, lipo) beside it
export PATH="$tbin:$PATH"
case " ${args[*]} " in *" -E "*|*" -M "*|*" -MM "*|*" -S "*|*" -fsyntax-only "*) one=1 ;; *) one=0 ;; esac
if [ ${#archs[@]} -eq 1 ] || [ $one = 1 ]; then
    exec "$(cc_for "${archs[0]}")" $(gccinc "${archs[0]}") "${args[@]}"
fi
if [ -z "$out" ]; then
    if [ $compile_only = 1 ]; then
        src=
        for a in "${args[@]}"; do case "$a" in *.c|*.m|*.cc|*.cpp|*.mm|*.S) src=$a ;; esac; done
        out=$(basename "${src%.*}").o
    else
        out=a.out
    fi
fi
tmp=$(mktemp -d "${TMPDIR:-/tmp}/maccc.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
base=(); skip=0
for a in "${args[@]}"; do
    if [ $skip = 1 ]; then skip=0; continue; fi
    case "$a" in -o) skip=1 ;; -o?*) ;; *) base+=("$a") ;; esac
done
pids=()
for arch in "${archs[@]}"; do
    "$(cc_for "$arch")" $(gccinc "$arch") "${base[@]}" -o "$tmp/$arch" &
    pids+=($!)
done
rc=0
for p in "${pids[@]}"; do wait "$p" || rc=1; done
[ $rc = 0 ] || exit 1
files=()
for arch in "${archs[@]}"; do files+=("$tmp/$arch"); done
"$tbin/i386-apple-darwin8-lipo" -create "${files[@]}" -output "$out"
DRIVER
    chmod +x "$MACCROSS_ROOT/bin/maccc"
    for n in gcc-4.0 gcc-4.2 cc gcc; do ln -sf maccc "$MACCROSS_ROOT/bin/$n"; done
    ln -sfn "$MACCROSS_ROOT/src/oxppc/target" "$MACCROSS_ROOT/target"
    for t in otool lipo ar ranlib strip nm size install_name_tool; do
        ln -sf "$MACCROSS_ROOT/target/bin/i386-apple-darwin8-$t" "$MACCROSS_ROOT/bin/$t"
    done
    # the Mac's md5 -q, python (the Macs' scripts say python), plutil -lint (nothing to check it with here)
    printf '#!/bin/sh\nif [ "$1" = -q ]; then shift; md5sum "$@" | cut -d" " -f1; else md5sum "$@"; fi\n' > "$MACCROSS_ROOT/bin/md5"
    printf '#!/bin/sh\nexit 0\n' > "$MACCROSS_ROOT/bin/plutil"
    chmod +x "$MACCROSS_ROOT/bin/md5" "$MACCROSS_ROOT/bin/plutil"
    ln -sf "$(command -v python3)" "$MACCROSS_ROOT/bin/python"
    echo "#include <stddef.h>" > "$MACCROSS_ROOT/shim/StdDef.h"
}

toolchain() {
    # The recipe that was run by hand (2026-10-18, Ubuntu 26.04, 24 cores): osxcross's ppc-test branch for cctools and
    # ld64 with PowerPC support, then Apple's gcc 4.2.1 (build 5575 for ppc, 5666.3 for i386) for each target.
    r=$MACCROSS_ROOT
    for s in MacOSX10.4u.sdk; do
        [ -d "$r/sdks/$s" ] || { echo "mac_cross_build: copy $s into $r/sdks first (see the top of this file)" >&2; exit 2; }
    done
    mkdir -p "$r/src/deps" "$r/host"
    cd "$r/src"
    [ -d oxppc ] || git clone -q --depth 1 -b ppc-test https://github.com/tpoechtrager/osxcross oxppc
    # gcc 4.2's generators need flex, bison and m4, which are built here when the host has none
    export PATH=$r/host/bin:$PATH
    cd "$r/src/deps"
    for p in "m4-1.4.19.tar.xz https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.xz" \
             "bison-3.8.2.tar.xz https://ftp.gnu.org/gnu/bison/bison-3.8.2.tar.xz" \
             "flex-2.6.4.tar.gz https://github.com/westes/flex/releases/download/v2.6.4/flex-2.6.4.tar.gz"; do
        set -- $p
        d=${1%.tar.*}
        if [ ! -x "$r/host/bin/${d%%-*}" ]; then
            [ -f "$1" ] || wget -q "$2"
            tar xf "$1"
            ( cd "$d" && ./configure --prefix="$r/host" CFLAGS="-O2 -std=gnu99" > /dev/null && make -j"$(nproc)" > /dev/null && make install > /dev/null )
        fi
    done
    cd "$r/src/oxppc/tarballs"
    tar czf MacOSX10.4u.sdk.tar.gz -C "$r/sdks" MacOSX10.4u.sdk
    for v in 5575.11 5666.3; do [ -f gcc-$v.tar.gz ] || wget -q -O gcc-$v.tar.gz https://opensource.apple.com/tarballs/gcc/gcc-$v.tar.gz; done
    cd "$r/src/oxppc"
    # cctools and ld64 (the build ends at a link test that the stock ld64 fails; the fixes below follow it)
    OCDEBUG=1 UNATTENDED=1 SDK_VERSION=10.4 ./build.sh > ../oxppc-build.log 2>&1 || true
    ld64=$(echo "$r"/src/oxppc/build/cctools-*/cctools/ld64)
    # the host's libstdc++ assertions stop ld64; PowerPC's 10.4 crt1.o has an empty __symbol_stub before the real
    # __symbol_stub1, and reaches dylib functions by the half-word address of their stubs
    python3 - "$ld64" <<'PATCH'
import sys
d = sys.argv[1]
f = d + '/src/ld/parsers/macho_relocatable_file.cpp'
s = open(f).read()
s = s.replace('if ( _stubsSectionNum == 0 ) {',
              'if ( (_stubsSectionNum == 0) || (_stubsMachOSection->size() == 0) ) { // port: crt1.o of 10.4', 1)
open(f, 'w').write(s)
f = d + '/src/ld/passes/stubs/stubs.cpp'
s = open(f).read()
old = """			default:
				if ( target->contentType() == ld::Atom::typeResolver ) {"""
new = """			default:
				// port: PowerPC crt1.o of Mac OS X 10.4 reaches dylib functions by the half-word address of their stubs
				if ( (fixup->kind == ld::Fixup::kindSetTargetAddress) && (target != NULL)
					&& (target->definition() == ld::Atom::definitionProxy) && (_architecture == CPU_TYPE_POWERPC) ) {
					for (const ld::Fixup* g = fixup; ; ) {
						if ( (g->kind == ld::Fixup::kindStorePPCAbsLow16) || (g->kind == ld::Fixup::kindStorePPCAbsHigh16)
							|| (g->kind == ld::Fixup::kindStorePPCAbsHigh16AddLow) || (g->kind == ld::Fixup::kindStorePPCAbsLow14) )
							return target;
						if ( g->lastInCluster() )
							break;
						++g;
					}
				}
				if ( target->contentType() == ld::Atom::typeResolver ) {"""
assert old in s
open(f, 'w').write(s.replace(old, new, 1))
PATCH
    ( cd "$ld64" && make clean > /dev/null 2>&1; make -j"$(nproc)" CXXFLAGS="-O2 -w -std=c++0x -fblocks -U_GLIBCXX_ASSERTIONS" \
        CPPFLAGS="-U_GLIBCXX_ASSERTIONS" > /dev/null && make install > /dev/null )
    # the compilers: gcc's own `make all` stops at the target libraries (not needed: the SDK and libgcc.a below
    # supply them), so the compilers are made and installed by hand
    sed -i 's#^    libcilkrts/runtime/sysdep-unix.c$#    libcilkrts/runtime/sysdep-unix.c || true#' build_gcc.sh
    export PATH=$r/src/oxppc/target/bin:$PATH
    mkdir -p "$r/vp"
    printf 'typedef void *vproc_transaction_t;\nstatic inline vproc_transaction_t vproc_transaction_begin(void*p){return 0;}\nstatic inline void vproc_transaction_end(void*a,vproc_transaction_t b){}\n' > "$r/vp/vproc.h"
    APPLE_GCC=1 POWERPC=1 UNATTENDED=1 JOBS="$(nproc)" ./build_gcc.sh > ../gcc-ppc-build.log 2>&1 || true
    ( cd build/gcc-gcc-5575.11/build_powerpc && make -j"$(nproc)" all-gcc > /dev/null 2>&1 || true; make install-gcc > /dev/null )
    APPLE_GCC=1 UNATTENDED=1 JOBS="$(nproc)" ./build_gcc.sh > ../gcc-x86-build.log 2>&1 || true
    o=$r/src/oxppc/target
    d=$r/src/oxppc/build/gcc-gcc-5666.3
    mkdir -p "$d/build_i386"
    ( cd "$d/build_i386" && CC="cc -std=gnu89" ../configure --target=i386-apple-darwin8 --with-sysroot="$o/SDK/MacOSX10.4u.sdk" \
        --with-ld="$o/bin/x86_64-apple-darwin8-ld" --with-as="$o/bin/x86_64-apple-darwin8-as" --disable-nls \
        --enable-languages=c,c++,objc,obj-c++ --enable-checking=release --disable-libstdcxx-pch --prefix="$o" \
        --with-system-zlib --with-gxx-include-dir=/usr/include/c++/4.0.0 > ../i386-conf.log 2>&1
      export CPATH=$r/vp
      make -k -j"$(nproc)" all-gcc > ../i386-make.log 2>&1 || true
      make -k install-gcc > ../i386-inst.log 2>&1 || true
      cp gcc/xgcc "$o/bin/i386-apple-darwin8-gcc-4.2.1"
      ln -sf i386-apple-darwin8-gcc-4.2.1 "$o/bin/i386-apple-darwin8-gcc" )
    write_driver
    echo "toolchain built in $r; try: $0 session"
}

need_prebuilt() {
    if [ ! -f "$repo/build/mac-game-tick-session-final38/engine_objects_i386.txt" ]; then
        echo "mac_cross_build: $repo/build lacks the Macs' gate objects (the top of this file says how to copy them)" >&2
        exit 2
    fi
}

session() {
    setup_env
    need_prebuilt
    out=${1:-build/mac-cross-session}
    cd "$repo"
    d=$(mktemp -d)
    : > "$d/map"
    # the generated bodies, once, before the two builds that read them at the same time
    sh tools/mac_audit_bodies.sh build/mac-audit > /dev/null
    for arch in ppc i386; do
        (
            MAC_ARCHS=$arch MAC_RENDERED_SESSION_BUILD_ONLY=1 MAC_RENDERED_SESSION_OUT="$out/$arch" \
                sh tools/mac_rendered_session_checks.sh "$d/map" "$d/map" "$d/map" > "$out.$arch.log" 2>&1
        ) &
    done
    rc=0
    for job in $(jobs -p); do wait "$job" || rc=1; done
    rm -rf "$d"
    for arch in ppc i386; do tail -2 "$out.$arch.log"; done
    if [ $rc != 0 ]; then echo "mac_cross_build: a build failed; see $out.ppc.log and $out.i386.log" >&2; exit 1; fi
    ls -l "$out/ppc/mac_rendered_session_ppc" "$out/i386/mac_rendered_session_i386"
}

controller() {
    setup_env
    cd "$repo/port/mac/controller_setup"
    MAC_CROSS=1 sh build.sh
}

app() {
    setup_env
    out=${1:-build/mac-cross-session}
    cd "$repo"
    MAC_CROSS=1 sh tools/mac_app_bundle.sh --engine-ppc "$out/ppc/mac_rendered_session_ppc" \
        --engine-i386 "$out/i386/mac_rendered_session_i386" --no-dmg --out "$repo/build/mac-app-cross"
}

case $cmd in
    toolchain) toolchain ;;
    setup) write_driver ;;
    session) session "$@" ;;
    controller) controller ;;
    app) app "$@" ;;
    *) sed -n '2,/^set -eu/p' "$0" | sed -n '2,24p' >&2; exit 2 ;;
esac
