#!/bin/sh
set -eu
# Generate tools/mac_scenario_scripts_boundary.py locally with Python 3 and
# stage build/mac-audit/mac_scenario_script_validators.h before legacy builds.
out=${MAC_SCRIPT_OUT:-build/mac-scenario-scripts}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mkdir -p "$out"
for arch in ppc i386;do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    objects=
    for unit in port/mac/native_scenario_scripts.c port/mac/engine_script_cache.c tools/mac_scenario_scripts_probe.c source/memory/data.c port/mac/engine_services.c port/mac/player_services.c;do
        name=$(basename "$unit" .c)
        gcc-4.0 -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -DHALO_LINUX_PLATFORM_LAYER -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/tag_files -Isource/memory -Iport/mac -Iport/linux/include -Ibuild/mac-audit \
            -include port/mac/engine_prefix.h -include build/mac-audit/mac_engine_semantics.h \
            -c "$unit" -o "$out/${name}_$arch.o"
        objects="$objects $out/${name}_$arch.o"
    done
    gcc-4.0 -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -Wl,-dead_strip -Iport/mac $objects port/mac/retail_cache.c -o "$out/probe_$arch"
done
"$out/probe_i386" "$@"
