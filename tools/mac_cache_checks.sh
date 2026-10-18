#!/bin/sh
# Run in a staged repository tree on the MacBook (Xcode 3 / Apple GCC 4.0).
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror -Iport/mac \
        port/mac/cache_endian.c tools/mac_cache_endian_smoke.c \
        -o "$mac_dir/cache_endian_$arch"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -fms-extensions -fshort-wchar -fno-strict-aliasing \
        -I. -Isource -Isource/cseries -Isource/math -Isource/tag_files \
        -Iport/linux/include -Iport/include -Iport/mac \
        port/mac/cache_endian.c tools/mac_cache_layouts.c -o "$mac_dir/cache_layouts_$arch"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror -Iport/mac \
        port/mac/retail_cache.c port/mac/retail_geometry.c port/mac/cache_endian.c tools/mac_retail_report.c \
        -o "$mac_dir/retail_report_$arch"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -Wall -Wextra -Werror -Iport/mac \
        port/mac/retail_cache.c port/mac/retail_geometry.c port/mac/retail_texture.c \
        port/mac/mac_bsp_renderer.c tools/mac_bsp_render.c \
        -framework OpenGL -o "$mac_dir/bsp_render_$arch"
done
"$mac_dir/cache_endian_i386" "$@"
"$mac_dir/cache_layouts_i386"
