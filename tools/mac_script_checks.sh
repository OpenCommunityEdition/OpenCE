#!/bin/sh
# First generate script_slots{,_legacy_layout,_legacy_store}.c with
# mac_script_probe.py on the modern host, then copy them to MAC_CHECK_DIR
# on the MacBook. This script builds both architectures and runs Intel control.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
for unit in script_slots script_slots_legacy_layout script_slots_legacy_store; do
    "$mac_cc" -arch ppc -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -fms-extensions -O2 -fno-strict-aliasing \
        "$mac_dir/$unit.c" -o "$mac_dir/${unit}_ppc"
done
# The legacy variants must still pass on Intel: they reproduce its original
# storage behavior and serve as a control for the PPC-only failures.
for unit in script_slots script_slots_legacy_layout script_slots_legacy_store; do
    "$mac_cc" -arch i386 -std=gnu89 -fms-extensions -O2 -fno-strict-aliasing \
        "$mac_dir/$unit.c" -o "$mac_dir/${unit}_i386"
    echo "Intel control: $unit"
    "$mac_dir/${unit}_i386"
done
