#!/bin/sh
# First generate/stage mac_text_body.h and mac_text_fonts.c with mac_text_boundary.py.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
for arch in ppc i386;do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    for unit in port/mac/engine_text_gl.c "$mac_dir/mac_text_fonts.c";do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp -I. -Iport/mac -Iport/linux/src -I"$mac_dir" -c "$unit" -o "$mac_dir/${name}_text_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp -I. -Iport/mac -Iport/linux/src -Wl,-dead_strip tools/mac_text_gl_probe.c "$mac_dir/engine_text_gl_text_${arch}.o" "$mac_dir/mac_text_fonts_text_${arch}.o" "$mac_dir/mac_shared_config_config_${arch}.o" "$mac_dir/engine_config_config_${arch}.o" "$mac_dir/tomlc17_config_${arch}.o" "$mac_dir"/*_musl_world_graph_${arch}.o -framework OpenGL -o "$mac_dir/text_gl_$arch"
done
HALO_HIGH_RES_TEXT=1 "$mac_dir/text_gl_i386"
