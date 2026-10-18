#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XNET_COMPAT_OUT:-build/mac-xnet-compat}
mkdir -p "$out"
for arch in ppc i386; do
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Wno-traditional -fms-extensions -fshort-wchar \
        -I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/include/xdk \
        -idirafter port/linux/include -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h -ffunction-sections -fdata-sections \
        -Dfast_ftol=engine_xnet_fast_ftol \
        -c port/mac/engine_xnet_compat.c -o "$out/xnet_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Wno-traditional -fms-extensions -fshort-wchar \
        -I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/include/xdk \
        -idirafter port/linux/include -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h -include port/mac/engine_xdk_prefix.h \
        -Dfast_ftol=engine_winsock_init_unused_fast_ftol -ffunction-sections -fdata-sections \
        -c port/mac/engine_winsock_init.c -o "$out/winsock_init_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror -Iport/mac \
        -c port/mac/mac_xnet_address.c -o "$out/address_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
        -c port/mac/engine_xnet_status.c -o "$out/status_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror -Iport/mac \
        -c port/mac/engine_host_misc.c -o "$out/host_misc_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Wno-traditional -fms-extensions -fshort-wchar -ffunction-sections -fdata-sections \
        -I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/include/xdk \
        -idirafter port/linux/include -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h \
        -Dfast_ftol=probe_xnet_fast_ftol -Wl,-dead_strip \
        tools/mac_xnet_compat_probe.c "$out/xnet_$arch.o" "$out/winsock_init_$arch.o" \
        "$out/address_$arch.o" "$out/status_$arch.o" "$out/host_misc_$arch.o" \
        -o "$out/probe_$arch"
done
"$out/probe_i386"
