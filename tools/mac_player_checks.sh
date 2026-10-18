#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mac_data=${MAC_PLAYER_DATA_SOURCE:-source/memory/data.c}
mac_suffix=${MAC_PLAYER_OUTPUT_SUFFIX:-}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    objects=
    for unit in source/game/players.c "$mac_data" source/text/unicode.c port/mac/engine_services.c port/mac/player_services.c tools/mac_engine_player_probe.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
            -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include port/mac/engine_prefix.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_player_${arch}.o"
        objects="$objects $mac_dir/${name}_player_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -Wl,-dead_strip $objects -o "$mac_dir/engine_player_${arch}${mac_suffix}"
    nm -u "$mac_dir/engine_player_${arch}${mac_suffix}" > "$mac_dir/player_imports_$arch.txt"
    if awk '{print $NF}' "$mac_dir/player_imports_$arch.txt" | grep -E '^_(wcs|wmem|wprintf|wscanf|fwprintf|fwscanf|swprintf|swscanf|vwprintf|vfwprintf|vswprintf|vwscanf|vfwscanf|vswscanf|fgetwc|fgetws|fputwc|fputws|getwc|getwchar|putwc|putwchar|ungetwc|fwide|mbtowc|mbrtowc|mbsrtowcs|wcstombs|wcrtomb|wctomb|towlower|towupper|isw|_w)' ; then
        echo "Unsafe Darwin wide-character import in native UTF-16 engine slice" >&2
        exit 1
    fi
    if nm -m -u "$mac_dir/engine_player_${arch}${mac_suffix}" | grep 'weak'; then
        echo "Unresolved weak engine import" >&2
        exit 1
    fi
done
"$mac_dir/engine_player_i386${mac_suffix}"
