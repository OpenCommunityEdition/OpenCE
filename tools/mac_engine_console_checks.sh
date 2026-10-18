#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
	fp=
	if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
	"$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
		-std=gnu89 -O2 $fp -Iport/mac \
	port/mac/engine_console.c tools/mac_engine_console_probe.c \
	-o "$mac_dir/engine_console_$arch"
done
"$mac_dir/engine_console_i386"
