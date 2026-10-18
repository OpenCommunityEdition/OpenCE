#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=build/mac-audit
mkdir -p "$out"
python tools/mac_resource_boundary.py
for arch in ppc i386;do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    objects=
    for unit in port/mac/shared_d3d_resources.c port/mac/shared_texture_layout.c port/mac/native_resource_memory.c tools/mac_cpu_resources_probe.c;do
        name=$(basename "$unit" .c)
        "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries -Iport/mac -Iport/include/xdk -idirafter port/linux/include \
            -include port/mac/engine_xdk_prefix.h -include port/mac/resource_c_runtime.h -include build/mac-audit/mac_engine_semantics.h -c "$unit" -o "$out/${name}_cpu_resources_$arch.o"
        objects="$objects $out/${name}_cpu_resources_$arch.o"
    done
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -Wl,-dead_strip $objects -o "$out/cpu_resources_$arch"
    nm -u "$out/cpu_resources_$arch" > "$out/cpu_resources_imports_$arch.txt"
    if nm -m -u "$out/cpu_resources_$arch" | grep weak;then exit 1;fi
done
"$out/cpu_resources_i386"
