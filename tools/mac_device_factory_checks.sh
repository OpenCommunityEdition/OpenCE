#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit/device-factory}
shared_dir=${MAC_SHARED_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
python tools/mac_device_state_view.py "$mac_dir/mac_device_state_body.inc" "$mac_dir/mac_device_state_defaults.inc"
python tools/mac_xgpu_texture_decode_view.py "$mac_dir/mac_xgpu_texture_decode_view.inc"
for arch in ppc i386; do
    mac_fp=; if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    objects=
    for unit in port/mac/engine_device_state.c port/mac/engine_device_factory.c \
        port/mac/engine_device_resources.c port/mac/engine_rasterizer_services.c \
        port/mac/engine_screen.c port/mac/native_bitmap_tags.c port/mac/engine_texture_cache.c \
        port/mac/engine_services.c \
        port/mac/native_resource_memory.c tools/mac_device_factory_probe.c \
        port/mac/mac_gl_visibility.c port/mac/mac_gl_program_cache.c port/mac/mac_render_timing.c port/mac/mac_vertex_upload.c port/mac/mac_index_decode_cache.c port/mac/bsp_decode_cache.c port/mac/mac_gpu_textures.c port/mac/mac_shader_source_cache.c port/mac/mac_bone_gather.c port/mac/mac_glsl120.c port/mac/mac_nv2a_vertex.c port/mac/mac_nv2a_pixel.c \
        port/mac/mac_xgpu_text.c source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.c; do
        name=$(basename "$unit" .c)
        if [ "$unit" = port/mac/mac_render_timing.c ] || [ "$unit" = port/mac/mac_vertex_upload.c ] || [ "$unit" = port/mac/mac_index_decode_cache.c ] || [ "$unit" = port/mac/bsp_decode_cache.c ] || [ "$unit" = port/mac/mac_bone_gather.c ]; then
            "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
                -std=gnu89 -O2 $mac_fp -Iport/mac -c "$unit" \
                -o "$mac_dir/${name}_device_factory_${arch}.o"
            objects="$objects $mac_dir/${name}_device_factory_${arch}.o"
            continue
        fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I"$mac_dir" -Ibuild/mac-audit -I. -Isource -Isource/cseries -Isource/math \
            -Isource/physics -Isource/tag_files -Isource/scenario -Isource/structures \
            -Iport/linux/src -Iport/linux/game -I"source/saved games" -Isource/units \
            -Isource/render -Isource/sound -Isource/main -Isource/interface -Isource/rasterizer \
            -Isource/cutscene -Isource/ai -Isource/cache -Isource/camera -Isource/bitmaps \
            -Isource/models -Isource/items -Isource/effects -Isource/memory -Isource/game \
            -Isource/input -Isource/objects -Iport/mac -Iport/linux/game \
            -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h \
            -include port/linux/include/halo_port_capacity.h \
            -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h \
            -include "$shared_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_device_factory_${arch}.o"
        objects="$objects $mac_dir/${name}_device_factory_${arch}.o"
    done
    for unit in port/mac/mac_input.c port/mac/mac_window.c port/mac/engine_messages.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -Wall -Wextra -Iport/mac -c "$unit" \
            -o "$mac_dir/${name}_device_factory_${arch}.o"
        objects="$objects $mac_dir/${name}_device_factory_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -Iport/mac -c port/mac/retail_geometry.c \
        -o "$mac_dir/retail_geometry_device_factory_${arch}.o"
    objects="$objects $mac_dir/retail_geometry_device_factory_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fno-strict-aliasing -Iport/mac -Wl,-dead_strip \
        -framework Carbon -framework AGL -framework OpenGL $objects \
        port/mac/retail_cache.c port/mac/retail_texture.c \
        "$shared_dir/shared_d3d_resources_cpu_resources_${arch}.o" \
        "$shared_dir/shared_texture_layout_cpu_resources_${arch}.o" \
        -o "$mac_dir/device_factory_$arch"
done
"$mac_dir/device_factory_i386" "$@"
