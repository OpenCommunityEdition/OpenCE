#!/bin/sh
# From the repository root on the MacBook; no Xcode GUI required.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_PROBE_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
compile_ppc() {
    "$mac_cc" -arch ppc -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -fms-extensions -fshort-wchar -fno-strict-aliasing \
        -fwrapv -mno-fused-madd -I. -Isource -Isource/cseries -Isource/math \
        -Iport/linux/include -Iport/include "$@"
}
compile_ppc -include tools/mac_compile_probe_prefix.h \
    -c source/math/real_math.c -o "$mac_dir/real_math.o"
compile_ppc -include tools/mac_compile_probe_prefix.h \
    -S source/math/real_math.c -o "$mac_dir/real_math.s"
compile_ppc tools/mac_abi_smoke.c tools/mac_abi_callee.c -o "$mac_dir/mac_abi_smoke"
compile_ppc -S tools/mac_abi_callee.c -o "$mac_dir/mac_abi_callee.s"
compile_ppc -Wall -Wextra tools/mac_capabilities.c -framework OpenGL \
    -o "$mac_dir/mac_glsl_probe"
file "$mac_dir/real_math.o" "$mac_dir/mac_abi_smoke" "$mac_dir/mac_glsl_probe"
