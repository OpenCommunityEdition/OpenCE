#!/bin/sh
# Builds tools/mac_math_determinism_probe.c with the vendored musl maths for
# the Mac targets (i386, ppc) and, with MAC_MATH_LINUX=1, Linux x86 clang -m32
# in the Linux build's flags, and prints each run's digests.
# Compare the outputs: they must be identical line for line.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_MATH_OUT:-build/mac-math}
mkdir -p "$out"
for arch in ${MAC_ARCHS:-i386 ppc}; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    objects=
    for unit in port/third_party/musl-math/src/*.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu99 -O2 $fp -w -Iport/third_party/musl-math/include \
            -include port/third_party/musl-math/include/libm.h \
            -c "$unit" -o "$out/${name}_$arch.o"
        objects="$objects $out/${name}_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu99 -O2 $fp -Iport/include tools/mac_math_determinism_probe.c $objects \
        -o "$out/mac_math_probe_$arch"
done
