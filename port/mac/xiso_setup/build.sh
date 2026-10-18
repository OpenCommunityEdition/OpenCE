#!/bin/sh
# Builds "OpenCE Setup.app" (ppc + i386, Mac OS X 10.4u SDK, gcc 4.0) and the
# test tool xiso_cli. Run on the Intel Mac with Xcode 3.x, in this directory.
# xiso.c is compiled unchanged from port/linux/src (XISO_C=...), beside the
# shim headers platform.h and posix.h that stand in for the port's own.
set -e
cd "$(dirname "$0")"
CC=${CC:-gcc-4.0}
SDK=${SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
XISO_C=${XISO_C:-../../linux/src}
APP="OpenCE Setup.app"
FLAGS="-arch ppc -arch i386 -isysroot $SDK -mmacosx-version-min=10.4 -O2 -Wall"
STAGE=build
rm -rf "$STAGE" "$APP"
mkdir -p "$STAGE"
cp "$XISO_C/xiso.c" "$XISO_C/xiso.h" shim/platform.h shim/posix.h xiso_install.h "$STAGE/"

cc_c() { $CC $FLAGS -std=gnu89 -I"$STAGE" -c "$1" -o "$2"; }
cc_c "$STAGE/xiso.c" "$STAGE/xiso.o"
cc_c xiso_shim.c "$STAGE/shim.o"
cc_c xiso_install.c "$STAGE/install.o"
cc_c xiso_cli.c "$STAGE/cli.o"
$CC $FLAGS -fno-objc-gc -x objective-c -std=gnu99 -I"$STAGE" -c main.m -o "$STAGE/main.o"

$CC $FLAGS -o xiso_cli "$STAGE/cli.o" "$STAGE/install.o" "$STAGE/shim.o" "$STAGE/xiso.o"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
$CC $FLAGS -o "$APP/Contents/MacOS/OpenCE Setup" "$STAGE/main.o" "$STAGE/install.o" "$STAGE/shim.o" \
	"$STAGE/xiso.o" -framework Cocoa
cp Info.plist "$APP/Contents/Info.plist"
printf 'APPL????' > "$APP/Contents/PkgInfo"
rm -rf "$STAGE"
lipo -info "$APP/Contents/MacOS/OpenCE Setup" xiso_cli
