#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mac_archs=${MAC_SOCKET_ARCHS:-ppc i386}
mac_run_archs=${MAC_SOCKET_RUN_ARCHS:-i386 ppc}
mkdir -p "$mac_dir"
for arch in $mac_archs; do
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -Wall -Wextra \
        -D__int64='long long' -D__fastcall= -fms-extensions -fshort-wchar \
        -I. -Isource -Isource/cseries -Isource/math -Isource/tag_files -Iport/mac -Iport/include/xdk -idirafter port/linux/include \
        -include port/mac/engine_xdk_prefix.h -c tools/mac_socket_prefix_witness.c -o "$mac_dir/socket_prefix_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -Wall -Wextra \
        -D__int64='long long' -D__fastcall= \
        -Iport/mac -Iport/include/xdk -Iport/linux/include -Iport/linux/src \
        -c port/mac/engine_sockets.c -o "$mac_dir/engine_sockets_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -Wall -Wextra \
        -Iport/linux/src -Iport/linux/include \
        -c port/linux/src/net_settings.c -o "$mac_dir/net_settings_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -Wall -Wextra \
        -Iport/linux/src -Iport/linux/include \
        -c port/linux/src/net_fault.c -o "$mac_dir/net_fault_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -Wall -Wextra \
        -Iport/mac -c port/mac/mac_xnet_address.c -o "$mac_dir/mac_xnet_address_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 -Wall -Wextra \
        -D__int64='long long' -D__fastcall= -DHALO_LINUX_PLATFORM_LAYER \
        -Iport/mac -Iport/include/xdk -Iport/linux/include \
        -Iport/linux/src \
        tools/mac_socket_probe.c "$mac_dir/engine_sockets_${arch}.o" \
        "$mac_dir/net_settings_${arch}.o" "$mac_dir/net_fault_${arch}.o" \
        "$mac_dir/mac_xnet_address_${arch}.o" \
        -lpthread -o "$mac_dir/socket_$arch"
done
# the byte order of the addresses is what differs between the two, so the ppc
# program is run as well (under Rosetta, where it is installed)
for arch in $mac_run_archs; do
    runner=; if [ "$arch" = ppc ]; then runner="arch -ppc"; fi
    echo "running $arch"
    $runner "$mac_dir/socket_$arch"
    # the settings are read once, so the system link addressing is its own run
    $runner "$mac_dir/socket_$arch" --net-settings
    # and so is each of the network faults (debug.network_latency and its
    # companions): held back, every one lost, every one damaged
    $runner "$mac_dir/socket_$arch" --net-fault 250 0 0
    $runner "$mac_dir/socket_$arch" --net-fault 0 100 0
    $runner "$mac_dir/socket_$arch" --net-fault 0 0 100
done
