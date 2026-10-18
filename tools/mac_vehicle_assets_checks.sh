#!/bin/sh
# Warthog dependencies only: owned tag closure, without invoking object_new.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    objects=
    for unit in tools/mac_constructor_storage.c tools/mac_constructor_time.c \
        tools/mac_cseries_string_test_adapter.c \
        source/items/items.c source/items/weapons.c source/memory/data.c source/memory/memory_pool.c \
        port/mac/native_actor_assets.c port/mac/native_physics_tags.c \
        port/mac/native_bitmap_tags.c port/mac/engine_texture_cache.c port/mac/native_light_tags.c \
        port/mac/native_antenna_tags.c \
        port/mac/native_object_tags.c port/mac/native_collision_tags.c port/mac/native_animation_tags.c \
        port/mac/native_scenario_objects.c source/rasterizer/rasterizer_geometry.c source/math/real_math.c \
        port/mac/engine_services.c port/mac/player_services.c port/mac/native_model_tags.c \
        port/mac/native_model_buffers.c port/mac/native_geometry_math.c port/mac/engine_tags.c \
        tools/mac_vehicle_asset_probe.c; do
        name=$(basename "$unit" .c)
        prefix="-include port/mac/engine_xdk_prefix.h"
        if [ "$unit" = source/rasterizer/rasterizer_geometry.c ];then
            prefix="-include port/mac/engine_prefix.h -include port/mac/native_geometry_math.h -include port/mac/engine_xdk_prefix.h"
        fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/rasterizer -Isource/tag_files \
            -Isource/memory -Isource/game -Isource/input -Isource/objects -Isource/items -Isource/physics \
            -Iport/linux/game -Iport/mac -idirafter port/linux/include -Iport/include/xdk \
            $prefix -include port/linux/include/halo_port_capacity.h -include port/include/halo_math.h \
            -include "$mac_dir/mac_engine_semantics.h" -c "$unit" -o "$mac_dir/${name}_vehicle_assets_$arch.o"
        objects="$objects $mac_dir/${name}_vehicle_assets_$arch.o"
    done
    for unit in port/third_party/musl-math/src/*.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp \
            -Iport/third_party/musl-math/include -include port/third_party/musl-math/include/libm.h \
            -c "$unit" -o "$mac_dir/${name}_musl_vehicle_assets_$arch.o"
        objects="$objects $mac_dir/${name}_musl_vehicle_assets_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
        -Iport/mac -Wl,-dead_strip $objects port/mac/mapped_file.c port/mac/retail_cache.c \
        port/mac/retail_texture.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c \
        port/mac/native_tags.c port/mac/retail_collision_model.c port/mac/retail_collision.c \
        port/mac/retail_animation.c port/mac/retail_animation_graph.c -o "$mac_dir/vehicle_assets_$arch"
done
"$mac_dir/vehicle_assets_i386" "$@"
