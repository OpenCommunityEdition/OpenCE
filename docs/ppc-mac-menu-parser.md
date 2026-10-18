# Native embedded menu XML parsing

`mac_menu_files_view.py` makes a source-checked native view of the shared
`menu_files.c` Expat reader. It keeps the actual element/attribute parsing,
validation, menu-data construction, logging and UTF-8-to-UTF-16 conversion.
The native view reads the 54 committed embedded XML files in
`port/assets/menus/menus.json`; byte arrays are generated as aligned bytes so
PPC and Intel parse the same XML. It does not include the XML overrides from a
user's `menus` folder or the shared file's PNG/OpenGL art path. Those require
native settings-folder and renderer owners, respectively.

`tools/mac_menu_parser_checks.sh` builds the shared Expat sources and this
view for PPC/i386 using GCC 4.0 and the Tiger SDK. The i386 probe passes 9
checks on the MacBook, and the PPC probe passes 9 checks on both G4s. All 54
embedded XML files produce 1,014 widgets, 1,557 strings and 93 bitmaps; the
probe also checks cached repeat-load identity and UTF-16 newline/non-ASCII
conversion. This verifies the parser data boundary, not game widget creation,
menu input, menu art or an interactive menu session.

The compiler compatibility header removes only Expat's deprecated-attribute
message argument, which Tiger's GCC 4.0 rejects. The Expat implementation and
menu parser remain the vendored/shared source. The generated parser is included
in the full tick link manifest, while art registration and drawing still need
their actual native resource owner.
