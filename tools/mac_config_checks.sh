#!/bin/sh
# Build the shared settings/parser unchanged except for the four file/path
# operations and logging boundary. No SDL implementation or engine ABI shim.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
sed -e 's/#include "platform.h"/#include "engine_config.h"/' \
    -e '/^#include <SDL3\/SDL.h>$/d' \
    -e 's/SDL_GetBasePath/mac_config_base_path/g' \
    -e 's/SDL_LoadFile/mac_config_load_file/g' \
    -e 's/SDL_SaveFile/mac_config_save_file/g' \
    -e 's/SDL_free/free/g' -e 's/platform_log/mac_config_log/g' \
    port/linux/src/port_config.c > "$mac_dir/mac_shared_config.c"
# GCC 4 has no C11 static_assert; retain the vendored expression as a typedef.
cat > "$mac_dir/mac_toml_assert.h" <<'ASSERT'
#define MAC_TOML_JOIN_(a,b) a##b
#define MAC_TOML_JOIN(a,b) MAC_TOML_JOIN_(a,b)
#define static_assert(expression,message) typedef char MAC_TOML_JOIN(mac_toml_assert_,__LINE__)[(expression)?1:-1]
ASSERT
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    for unit in "$mac_dir/mac_shared_config.c" port/mac/engine_config.c port/third_party/tomlc17/tomlc17.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu99 -O2 $fp -DHALO_MAC -Iport/mac -Iport/linux/src -Iport/third_party/tomlc17 \
            -include "$mac_dir/mac_toml_assert.h" -c "$unit" -o "$mac_dir/${name}_config_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -Iport/linux/src tools/mac_config_probe.c \
        "$mac_dir/mac_shared_config_config_$arch.o" "$mac_dir/engine_config_config_$arch.o" \
        "$mac_dir/tomlc17_config_$arch.o" -o "$mac_dir/config_$arch"
done
