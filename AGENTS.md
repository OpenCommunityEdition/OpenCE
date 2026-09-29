# Agent Guide for halo-ce-universal

This repository is a port of a Halo: Combat Evolved (build 2342, `cachebeta.exe`) decompilation from Xbox/Windows to Linux, with additional Windows and Android ports. It is primarily a C89/C90 reverse-engineering and byte-matching project.

## What This Repository Is

- A decompilation of Halo 1 build 2342 (`cachebeta.exe`, sha256 `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`).
- The code targets the original Xbox development kit (XDK) ABI and the VC7 (Visual C++ 7.0 / 13.00.9254.1) compiler.
- The project has two largely independent build graphs:
  1. A **byte-matching** build that compiles source with the original MSVC/XDK toolchain and checks against the original `cachebeta.exe`.
  2. **Native ports** (Linux, Windows, Android) that compile the same game sources with clang plus a platform layer.
- The codebase is organized to keep the game logic byte-identical to the original while layering modern platform support on top.

## Repository Layout

```text
source/                 # Decompiled C game sources, organized by subsystem
  cseries/              # Core types, macros, memory, and platform abstractions
  main/                 # Entry point and main loop
  game/                 # Game state, engines, rules, players
  objects/              # Object system, datum indices, definitions
  units/                # Bipeds, vehicles, units, dialogue
  ai/                   # AI system
  physics/              # Collision, BSP, physics
  render/               # High-level rendering
  rasterizer/           # Low-level rasterizer and Xbox D3D interactions
  interface/            # HUD, UI widgets, menus
  networking/           # System link / LAN networking
  sound/                # Audio
  bitmaps/              # Bitmap and texture handling
  tag_files/            # Tag file parsing
  memory/               # Memory pools, caches, data structures
  math/                 # Math utilities
  shell/                # Xbox shell abstractions
port/                   # Platform ports
  linux/                # Linux port source and README
  windows/              # Windows port source and README
  android/              # Android port source, Gradle project, and README
  third_party/          # tomlc17 (shared config parser)
config/                 # Build configuration and matching metadata
  config.json           # Object list, per-TU compiler flags, include dirs
  splits.json           # Code splitting configuration
  relocs.json           # Relocation hints
  symbols.json          # Symbol ownership and aliases
  semantic_matches.json # Manual semantic match allowlist
  parked.json           # Parked (intentionally uncompleted) functions
  contribs.json         # Contribution/credit ledger
  object_admission_rejections.json
  semantic_credit_rejections.json
  regression_adjudications_*.json
tools/                  # Python tooling for build, audits, tests, analysis
  project_x86.py        # Main ninja/solution generator
  linux_build.py        # Native Linux build rules
  windows_build.py      # Native Windows build rules
  android_build.py      # Native Android build rules
  fake_match_scan.py   # Lexical scan for suspicious source constructs
  test_*.py             # pytest-based tests for tooling and runtime contracts
  audit/                # Runtime differential audit scripts
  fixtures/             # C fixtures for toolchain tests
  campaign/             # Campaign workflow helpers
docs/                   # Project documentation and matching notes
pgo/                    # Profile-guided optimization profiles
build/                  # Generated build artifacts (created by ninja)
projects/               # Generated per-object project files
xbox/                   # Xbox SDK headers (you must provide these externally)
```

## Build System & Essential Commands

All builds go through `configure.py` (which generates a `build.ninja` and `objdiff.json`) and then `ninja`.

### Prerequisites

- Python 3 and `ninja-build` on PATH.
- The August 2001 Xbox SDK. Extract `XDK/xbox` into the repo root so that `xbox/{bin,include}` exist.
- For the byte-matching build on Linux: the original `cachebeta.exe` in the repo root.

### Byte-Matching Build

```bash
python configure.py       # Generates build.ninja and objdiff.json
ninja                       # Compile all objects and produce build/report.json
```

- On Linux the build runs the XDK compiler under [wibo](https://github.com/decompals/wibo) (downloaded automatically) and assembles CRT `.asm` units with UASM when MASM is unavailable.
- Progress and comparison reports are produced in `build/report.json`.

### Native Linux Build

```bash
ninja linux                 # Produces build/linux/halo (32-bit ELF)
build/linux/halo            # Run the game; expects PAL data under assets/
```

- Requires clang, 32-bit glibc dev files, and 32-bit SDL3.
- Renders with OpenGL and uses SDL3 for audio/input.

### Native Windows Build

```bash
ninja windows               # Run on Windows; produces build/windows/halo.exe
```

- Uses the same shared platform layer as Linux plus Windows-specific files.

### Android Build

```bash
ninja android_apk           # Produces port/android/app/build/outputs/apk/debug/app-debug.apk
```

- The guest half (`ninja android`, `build/android/halo_guest.elf`) is built by the ninja graph and needs no SDK; only the Gradle step that wraps it into an APK needs one.
- The Android SDK is **not** installed system-wide on this machine. It lives in the sibling project checkout:

  ```text
  /home/carlo/projects/Wiicompiled/.android-sdk
  ```

  It carries `platforms/android-34` and `android-35`, `build-tools/34.0.0` and `36.0.0`, `ndk/26.1.10909125`, `cmdline-tools/latest` and accepted licenses. There is also a second, older NDK at `/home/carlo/projects/Wiicompiled/.android-ndk/r27c`, which the Android port does not use.

- The Android builds here are **release** builds (`--release`, which defines
  `HALO_RELEASE` and strips the assertions). Configure them with the NDK and
  the compilers named, or the build quietly becomes a debug build that behaves
  differently on a device:

  ```bash
  python configure.py --release --android-ndk /home/carlo/projects/Wiicompiled/.android-sdk/ndk/26.1.10909125 \
      --android-guest-cc clang-22 --linux-cc clang-22
  ```

  The guest image is the check that the configuration is right: a correct
  `build/android/halo_guest.elf` starts `3082cd20` when only the host changes.

- Gradle finds it through `port/android/local.properties` (gitignored, machine-local):

  ```properties
  sdk.dir=/home/carlo/projects/Wiicompiled/.android-sdk
  ```

  Without it, and with `ANDROID_HOME`/`ANDROID_SDK_ROOT` unset, `ninja android_apk` fails at `:app:compileDebugJavaWithJavac` with "SDK location not found". Set `ANDROID_HOME` instead of the file if you prefer not to keep a local path in the tree.
- `port/android/app/build.gradle` pins `compileSdk 35` and only `arm64-v8a`, and the guest is built as `arm64_32`.
- The guest ELF is packaged as `assets/halo_guest.elf` with `noCompress "elf"`; its `md5sum` should match `build/android/assets/halo_guest.elf` if you want to confirm an APK carries the code you just built.

### Other Configure Flags

```bash
python configure.py --release        # Release builds for native ports (assertions disabled)
python configure.py --portable       # Target x86-64 baseline (SSE2) instead of -march=native
python configure.py --lto=thin       # Thin LTO; choices: full/thin/off
python configure.py --pgo=train      # Record new PGO profile (needs assets/ and display)
python configure.py --pgo=off         # Disable PGO
python configure.py --linux-cc <cc>  # Override the Linux build compiler (default: clang)
python configure.py --wrapper <path> # Use wibo/wine for the XDK compiler
python configure.py --ml <path>      # Path to MASM for authentic CRT .asm units
python configure.py --objdiff <path> # Use a local objdiff-cli binary
python configure.py --csplit <path>  # Use a local csplit binary
```

### Progress Report

```bash
python configure.py progress         # Print progress information from config
```

## Byte-Matching Workflow

The byte-matching workflow is the core of the project:

1. Each `.c`/`.asm` unit is compiled with the original MSVC/XDK toolchain.
2. `objdiff-cli` compares each compiled object against the original `cachebeta.exe`.
3. Results are written to `build/report.json`.
4. `config/config.json` records each object's status: `MISSING`, `NonMatching`, or `Matching`.
5. Exact-matched units are recorded; partial/fuzzy units are analyzed and either improved or parked.

Key files:

- `config/config.json` - source file list, include dirs, per-TU compiler flags.
- `config/symbols.json` - symbol ownership and aliases.
- `config/semantic_matches.json` - manual allowlist for functions that match semantically but may differ structurally.
- `config/parked.json` - functions deliberately left non-matching, with reason and class.
- `config/object_admission_rejections.json` - objects rejected from admission.
- `config/semantic_credit_rejections.json` - semantic credits rejected after review.
- `build/report.json` - generated comparison report.
- `build/semantic_report.json` - generated semantic progress report.

## Code Conventions & Style

### Language & ABI

- Source is **C89/C90 with MSVC extensions** for the game code.
- Native ports use C11 for the platform layer.
- The game targets the original Xbox/Win32 ABI:
  - `real` is `float`.
  - `boolean` is `unsigned char` (one byte).
  - `tag` is `unsigned long`.
  - `wchar_t` is 16-bit (`-fshort-wchar`).
  - Structs use MSVC layout (`-malign-double`, `-fms-extensions`).
  - Tentative definitions are shared between translation units (`-fcommon`).
  - The compiler keeps EBP frames (`-fno-omit-frame-pointer`).

### Formatting

- `source/.editorconfig` uses tabs, indent size 4, and CRLF line endings for C/C++ files.
- Style is documented in the editorconfig; no separate formatter is configured.

### Naming Conventions

- Private translation-unit helpers and globals should have authentic or descriptive names.
- Do **not** add `code_` + address or `bss_` + address identifiers (house rule).
- Use project types (`real`, `boolean`, `tag`, etc.) rather than generic types.
- Bit flags are usually declared as enum bit **indices** and converted with `FLAG(bit)`.
- Use subsystem-typed macros wrapping `tag_get` (e.g., `object_definition_get`).
- Use typed object-access macros instead of repeated raw casts.

### Assertion & Diagnostic API

Defined in `source/cseries/cseries.h` and documented in `docs/assertions.md`:

- `assert(expr)` - fatal if false.
- `dassert(expr, diagnostic)` - fatal with custom diagnostic string.
- `vassert(expr, csprintf(temporary, ...))` - fatal with formatted diagnostic, only on failure.
- `warn(expr)` / `dwarn(expr, diagnostic)` / `vwarn(...)` - nonfatal warnings.
- `halt()` / `dhalt(diagnostic)` / `vhalt(...)` - unconditional fatal.
- Matching forms (`match_assert`, `match_vassert`, etc.) preserve the original file/line.

Important: the original VC7 compiler does **not** support variadic macros. Format strings are built with `csprintf(temporary, ...)`.

### Header Organization

- Each subsystem has its own directory.
- Headers follow the pattern: constants, macros, structures, prototypes.
- Put prototypes in their genuine owning headers, not in unrelated consumer `.c` files.
- When no dedicated header exists, use the closest associated header.

## Testing & Validation

### pytest Suite

Most Python tooling tests use pytest and are in `tools/test_*.py`. Run with:

```bash
python -m pytest -q tools
```

Some tests require `unicorn` and are skipped if it is not installed.

### Fake-Match Scan

```bash
python tools/fake_match_scan.py --help
```

This is a conservative lexical scanner for suspicious constructs (self-assignments, empty branches, fixed boolean conditions, inline assembly, raw byte emission, etc.). It produces review leads, not verdicts.

### Runtime Differential Audits

Scripts in `tools/audit/` replay specific functions from the original binary against reconstructed code and check for ABI/state drift. They are run as part of the validation workflow but are narrowly scoped to specific functions.

### Campaign/Batch Gate

`tools/campaign/` contains helpers used during matching campaigns (attestation scans, merge candidate tools, relocation diff tools, etc.).

## Documentation & Quick References

Key documents an agent should consult before making changes:

- `README.md` - high-level build instructions and project overview.
- `docs/campaign_house_rules.md` - current campaign rules and working cadence.
- `docs/common_constants.md` - canonical constants, types, float patterns, tag IDs, and flag conventions.
- `docs/assertions.md` - shared assertion macros and matching examples.
- `docs/matching_methodology.md` - exact-matching methodology and blocker classes.
- `docs/user_source_reconstruction_map_20260906.md` - reconstruction map from supplied source fragments.
- `port/linux/README.md`, `port/windows/README.md`, `port/android/README.md` - platform-specific build/run instructions.

## Important Gotchas

- **Do not assume modern C semantics.** The target compiler is VC7 from 2001/2002. Avoid variadic macros, C99 features, or modern compiler extensions in game code.
- **Exact matching is the primary goal.** Do not invent source shapes merely to make code compile or look cleaner; changes must be backed by relocation, instruction, and semantic evidence.
- **Preserve the MSVC ABI.** Native builds deliberately compile with `-fms-extensions`, `-fshort-wchar`, `-malign-double`, `-fcommon`, and `-fno-omit-frame-pointer` to match the original layout and calling conventions.
- **Assertions differ between debug and release.** `configure.py --release` defines `HALO_RELEASE` and strips assertion checks; diagnostic expressions are still evaluated because some code relies on side effects.
- **Use the matching assertion forms when preserving original locations.** `match_assert(file, line, expr)` reports the original source line, not the current one.
- **Bit flags are indices, not masks.** Use `FLAG(_some_bit)` to turn an enum bit index into a mask. Do not apply `FLAG` to values that are already masks.
- **Data layout matters.** Changing a struct field order, padding, or type width can break exact matching even if the game still runs.
- **Parked functions are intentional.** If a function is listed in `config/parked.json`, do not try to make it exact without reviewing the recorded reason.
- **Semantic matches are allow-listed.** A function can be marked exact via `config/semantic_matches.json` only after a hardened semantic audit; do not add entries lightly.
- **Avoid fake matching.** The project explicitly rejects source that is plausible only because it happens to produce the right bytes. Source must be coherent program logic.
- **PGO profiles are architecture-specific.** `pgo/halo_linux.profdata` and `pgo/halo_windows.profdata` are checked in; training requires a display and the game data in `assets/`.
- **The build downloads tools automatically.** `wibo`, `objdiff-cli`, `csplit`, and `uasm` are downloaded/built on demand into `build/tools/` unless overridden by configure flags.
