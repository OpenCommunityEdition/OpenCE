#!/bin/sh
# mac.render_resolution's choices (port/mac/mac_render_scale.h, mac_gpu_pick.h), no graphics card needed.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
	fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
	"$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
		-std=gnu89 -O2 $fp -Iport/mac tools/mac_render_scale_probe.c \
		-o "$mac_dir/render_scale_$arch"
done
"$mac_dir/render_scale_i386"
