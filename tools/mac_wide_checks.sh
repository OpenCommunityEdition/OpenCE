#!/bin/sh
# Whole genuine UTF-16 implementation, explicit header routing and native files.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
sed -e 's/#include "platform.h"/#include "engine_wide.h"\
typedef int BOOL;\
#define TRUE 1\
#define FALSE 0/' \
    -e '/^#include <wchar.h>$/d' \
    -e 's/static __thread wchar_t \*next;/wchar_t **mac_next = mac_wide_tokenizer_state();\
#define next (*mac_next)/' \
    -e '/^wchar_t \*msvc_wcsdup/i\
#undef next\
' \
    -e 's/halo_linux_fopen/fopen/g' -e 's/halo_linux_freopen/freopen/g' \
    -e 's/halo_linux_remove/remove/g' port/linux/src/msvc_wide.c > "$mac_dir/mac_shared_wide.c"
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    for unit in "$mac_dir/mac_shared_wide.c" port/mac/engine_wide.c port/mac/engine_crt.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp -fshort-wchar -Iport/mac -c "$unit" -o "$mac_dir/${name}_wide_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -fshort-wchar -Iport/mac tools/mac_wide_probe.c \
        "$mac_dir/mac_shared_wide_wide_$arch.o" "$mac_dir/engine_wide_wide_$arch.o" "$mac_dir/engine_crt_wide_$arch.o" \
        -o "$mac_dir/wide_$arch"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -fshort-wchar -I. -Iport/mac -DHALO_MAC_ENGINE_WIDE_H \
        -include port/mac/engine_prefix.h -include port/mac/engine_wide_redirect.h \
        tools/mac_wide_probe.c "$mac_dir/mac_shared_wide_wide_$arch.o" \
        "$mac_dir/engine_wide_wide_$arch.o" "$mac_dir/engine_crt_wide_$arch.o" -o "$mac_dir/wide_redirect_$arch"
    if nm -u "$mac_dir/wide_$arch" "$mac_dir/wide_redirect_$arch" | egrep '_(wcs|wmem|vsw|vsnw|swprintf|fgetwc|fputwc|tow|isw)' ; then
        echo "unexpected host wide-character import" >&2; exit 1
    fi
done
