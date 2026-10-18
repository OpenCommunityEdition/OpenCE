#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_MENU_PARSER_OUT:-build/mac-audit/menu-parser}
mkdir -p "$out"
python tools/mac_menu_files_view.py "$out/mac_menu_parser.c" "$out/mac_menu_xml.c"
printf '#define __deprecated__(message)\n' > "$out/mac_expat_compat.h"
for arch in ppc i386; do
	flags="-arch $arch -isysroot $sdk -mmacosx-version-min=10.4"
	if [ "$arch" = ppc ]; then fp=-mno-fused-madd; else fp=; fi
	for unit in "$out/mac_menu_parser.c" "$out/mac_menu_xml.c" tools/mac_menu_parser_probe.c; do
		name=$(basename "$unit" .c)
		if [ "$unit" = "$out/mac_menu_parser.c" ]; then compat="-include $out/mac_expat_compat.h"; else compat=; fi
		$cc $flags -std=gnu89 -O2 $fp $compat -I. -Iport/linux/src -Iport/third_party/expat \
			-c "$unit" -o "$out/${name}_$arch.o"
	done
	for unit in xmlparse xmlrole xmltok; do
		$cc $flags -std=gnu99 -O2 $fp -include "$out/mac_expat_compat.h" -Iport/third_party/expat \
			-c "port/third_party/expat/$unit.c" -o "$out/${unit}_$arch.o"
	done
	$cc -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -Wl,-dead_strip \
		"$out/mac_menu_parser_$arch.o" "$out/mac_menu_xml_$arch.o" "$out/mac_menu_parser_probe_$arch.o" \
		"$out/xmlparse_$arch.o" "$out/xmlrole_$arch.o" "$out/xmltok_$arch.o" -o "$out/probe_$arch"
done
"$out/probe_i386"
