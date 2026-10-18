#!/bin/sh
# Builds "OpenCE Controller Setup.app" (ppc + i386, Mac OS X 10.5 SDK).
# Run on the Intel Mac with Xcode 3.x, in this directory. Also builds and runs test_config.
set -e
cd "$(dirname "$0")"
CC=${CC:-gcc-4.2}
SDK=${SDK:-/Developer/SDKs/MacOSX10.5.sdk}
APP="OpenCE Controller Setup.app"
FLAGS="-arch ppc -arch i386 -isysroot $SDK -mmacosx-version-min=10.5 -O2 -Wall -std=gnu89"

$CC -o test_config.bin test_config.c controller_setup_config.c controller_setup_hid.c -std=gnu89 -Wall
./test_config.bin
rm -f test_config.bin

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
# .m files take gnu99 for Objective-C declarations; the C files are gnu89.
$CC $FLAGS -fno-objc-gc -x objective-c -std=gnu99 -c main.m -o main.o
$CC $FLAGS -c controller_setup_config.c -o config.o
$CC $FLAGS -c controller_setup_hid.c -o hid.o
$CC $FLAGS -o "$APP/Contents/MacOS/OpenCE Controller Setup" main.o config.o hid.o \
	-framework Cocoa -framework IOKit -framework Security
rm -f main.o config.o hid.o
cp Info.plist "$APP/Contents/Info.plist"
printf 'APPL????' > "$APP/Contents/PkgInfo"
lipo -info "$APP/Contents/MacOS/OpenCE Controller Setup"
