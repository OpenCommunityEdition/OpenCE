#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
# Deliberate stock constructor configuration: do not define enhanced-network
# HALO_PORT_MAXIMUM_NETWORK_PLAYERS. Its projectile-trail precache requires a
# complete projectile/contrail/render dependency closure unavailable here.
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ]; then fp=-mno-fused-madd;fi
    objects=
    for unit in tools/mac_constructor_storage.c tools/mac_constructor_time.c source/items/items.c source/items/weapons.c source/memory/data.c source/memory/memory_pool.c port/mac/native_object_tags.c port/mac/engine_tags.c port/mac/engine_services.c port/mac/player_services.c port/mac/pool_services.c tools/mac_weapon_constructor_probe.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/memory -Isource/game -Isource/input -Isource/objects -Isource/items -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_capacity.h -include port/mac/engine_xdk_prefix.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_constructor_$arch.o"
        objects="$objects $mac_dir/${name}_constructor_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac -Wl,-dead_strip \
        $objects port/mac/retail_cache.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c -o "$mac_dir/weapon_constructor_$arch"
    if nm -m -u "$mac_dir/weapon_constructor_$arch" | grep 'weak'; then echo 'Unresolved weak engine import' >&2;exit 1;fi
done
"$mac_dir/weapon_constructor_i386" "$@"
