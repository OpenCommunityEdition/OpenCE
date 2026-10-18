#!/bin/sh
# Run from the repository root with Apple GCC 4.0 (Xcode 3).
# Produces a PPC executable; run it on a PowerPC Mac, not under Rosetta.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_output=${MAC_OUTPUT:-build/mac-audit/mac_byteswap_smoke-ppc}
mkdir -p "$(dirname "$mac_output")"
# This isolated unit does not use cseries.h's extern TLS variable. Erasing
# its declaration's TLS qualifier is only for this harness, not a game fix.
# Do not globally redefine __inline: it breaks Darwin's system headers.
"$mac_cc" -arch ppc -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
    -std=gnu89 -fshort-wchar -fno-strict-aliasing -fwrapv -mno-fused-madd \
    '-D__int64=long long' -D__thread= \
    -Isource/cseries -Isource/memory -Iport/linux/include \
    source/memory/byte_swapping.c tools/mac_byteswap_smoke.c -o "$mac_output"
file "$mac_output"
