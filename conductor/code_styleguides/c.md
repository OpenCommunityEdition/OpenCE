# C Style Guide (OpenCE)

The game's sources (`source/`) are the decompilation. The port's C (`port/linux/src`, `port/linux/game`, the guest runtime) is written to match them.

## Language
- C89 (`gnu89`). Declarations come before statements in each block.
- The guest is ILP32 (`arm64_32`): `int`, `long` and pointers are 32 bits. Never assume a pointer fits in or equals a 64-bit host value. Values that cross to the host go through the import stubs.
- Keep the MSVC-style extensions the sources use (`-fms-extensions -fshort-wchar -fcommon`). Don't add new dependencies on them.

## The game's sources
- Change `source/` as little as possible. Start each change with a `port:` comment that says what changed and why.
- Keep `BUG (original, preserved for exact matching)` notes. If you fix one, say so in the `port:` comment.
- Declare port functions that the game calls in `halo_linux_source_fixups.h`, or include their header with a note naming the file:
  `#include "view_fov.h" /* port: port/linux/game/view_fov.c */`
- Guard code for one platform with that platform's define. `HALO_LINUX` and `HALO_CUSTOM_EDITION` are never defined.

## Naming and layout
- Lower-case `snake_case` for functions, variables and types, following the sources' module prefixes (`network_`, `map_share_`, `custom_edition_`).
- Match the surrounding file's brace style, indentation and comment density.
- Comments are plain and descriptive: what the code does, and why when it isn't obvious.

## Determinism and maths
- Use the vendored musl maths (`halo_math.h`). Never call the system's maths functions.
- Compile without fused multiply-add. Don't add code whose results depend on the compiler or the CPU (uninitialised reads, unspecified evaluation order, float-to-int of out-of-range values).

## Untrusted input
- Read map files, mods and network packets only through bounds-checked readers. Check sizes, indices and rates before using them, as the existing handlers do.
- Never trust a length or an index read from a file or packet.

## Settings
- A new behaviour that departs from the stock game is a setting in `port/linux/src/port_config.c`: with a default (off or stock), an environment variable and a comment, and documented in the settings table of `port/linux/README.md`.
