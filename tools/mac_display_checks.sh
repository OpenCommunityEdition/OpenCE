#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
	fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
	"$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
		-std=gnu89 -O2 $fp -Iport/mac port/mac/engine_display.c \
		tools/mac_display_probe.c -framework ApplicationServices \
		-o "$mac_dir/display_$arch"
done
"$mac_dir/display_i386"
