# Notes for agents

This repository ports the Halo: Combat Evolved decompilation (Xbox build
01.01.14.2342, `cachebeta.exe`) to Linux, Windows, Android and the web. The game's C
sources are the decompilation; the port adds a platform layer that
implements the Xbox APIs the game calls, and features the Xbox game did not
have (a new netcode, internet play, the PC version's menus, Custom Edition
maps, touch controls). The project is published as OpenCE.

Start with [README.md](README.md). The other documents are listed at the
end.

## GulchCE instructions for every assignment

Read this file before starting each assignment and explicitly confirm your
understanding of its requirements. Then review the isolation rules below
and explicitly confirm your understanding of them before proceeding.
Read [CODINGSTANDARDS.md](CODINGSTANDARDS.md) for supplementary working and
review guidance. Those standards do not relax the isolation rules.

### Applying these requirements

- The common project requirements apply to every assignment, including
  building, testing, networking, licensing, source provenance and Git
  conventions.
- Before implementation, identify whether the assignment adds a general
  feature, implements MCC compatibility, fixes existing behavior, merges
  upstream changes or combines these activities. Briefly state which
  isolation requirements apply.
- General feature work must follow the general feature isolation rules.
  MCC compatibility work must additionally follow the MCC isolation rules;
  the general rules do not relax the stricter MCC boundaries.
- For mixed assignments, apply the relevant rules to each affected part.
  Classify work by what it changes, not merely by the assignment's title.
  A general feature request does not authorize unrelated MCC compatibility
  changes or changes to map-family boundaries.
- When shared-engine work can affect multiple map families, inspect and
  validate the affected Xbox, Custom Edition and MCC paths. Incidental use
  of a shared engine service by MCC does not make all general engine work
  MCC-specific, but existing MCC guards, ownership and cleanup must remain
  intact.
- These rules provide standing permission for necessary integration that
  meets their conditions within an authorized assignment. Explain the
  rationale and perform the required validation without requesting separate
  approval for routine compliant changes. If a specific change conflicts
  with an isolation rule, stop only the affected part, explain the conflict,
  and continue independent work that remains within scope. A general
  request to complete a feature does not itself waive an isolation rule.

### General feature isolation

- Preserve the established Halo 1 Xbox and Custom Edition implementations.
  Do not replace, rename, remove, obscure or repurpose their code, scripts,
  variables or loading paths to implement a new feature. The narrowly
  scoped additive integration described below is allowed.
- Keep feature logic in dedicated feature modules, extending an existing
  feature-owned module where appropriate. Use existing engine interfaces
  where possible. Minimal registration and wiring in build files, settings,
  menus and declarations are allowed when necessary for the authorized
  feature and must follow the applicable project rules. This does not
  authorize moving feature logic into established engine implementations.
  Keep MCC-specific compatibility logic in MCC-owned modules.
- Optional features must preserve established behavior when disabled.
  They must default to off or stock behavior unless the user explicitly
  authorizes a different default. When enabled, changes must remain limited
  to the authorized feature; unrelated behavior and map-family boundaries
  must be preserved. Follow the Settings requirements below for features
  that depart from the stock game's look or behavior.
- Existing implementations may be analysed for reference. This does not
  authorize replacing or repurposing them for new feature compatibility.
  Follow the third-party licensing, source-provenance and Xbox SDK rules
  in this file and the coding standards.
- Minimal additive shared-engine hooks, declarations or extension points
  are allowed only when strictly necessary for authorized functionality
  that cannot be implemented through existing interfaces in feature
  modules. Keep feature logic in feature-owned modules and shared additions
  as small as possible. Guard feature-specific execution and state with
  the appropriate activation conditions.
- Handle initialization and cleanup at the feature's relevant lifecycle
  boundaries. These may include enabling or disabling the feature, map
  loading and unloading, failed loads, session changes and shutdown.
  Feature state must not leak into later sessions or unaffected paths.
- For each shared addition, explain why a feature-module-only solution is
  insufficient, identify the affected engine paths, and mark changes to
  `source/` with `port:` as required below. Verify established behavior
  with the feature disabled, the intended behavior with it enabled, and
  relevant lifecycle and map-family transitions.
- This permission covers necessary integration, not replacing, renaming
  or refactoring established implementations for convenience or code
  reduction. Existing hooks are not blanket permission for broader
  changes. Refactoring, deduplication, cleanup and line-count targets are
  not exceptions.
- If a feature cannot fit within these constraints, explain the specific
  conflict and stop that part of the work. Do not proceed unless the user
  explicitly changes the applicable constraint.
- Report the actual validation performed and its limits. Scale checks to
  the affected behavior without waiving required platform or networking
  checks. A successful load or green test suite is not proof of full
  compatibility.

### MCC isolation

- Preserve the established Halo 1 Xbox and Custom Edition implementations
  and functionality. Do not change, rename, remove, obscure or repurpose
  their code, scripts, variables or loading paths to implement MCC support.
  The narrowly scoped shared-engine additions described below are allowed;
  they must preserve the existing Xbox and Custom Edition logic and behavior.
- MCC support must remain independently implemented in dedicated MCC
  modules. Xbox maps use `maps`, Custom Edition maps use `custom_maps`, and
  MCC maps and their package folders use `mcc_maps`.
- Existing Xbox and Custom Edition code may be analysed for reference;
  this does not authorize editing or hijacking it for MCC compatibility.
  Do not copy proprietary MCC code or prohibited SDK material. Follow the
  third-party licensing and source-provenance rules below and in the
  coding standards.
- Minimal additive shared-engine hooks, declarations or extension points
  are allowed only when strictly necessary for authorized MCC functionality
  that cannot be implemented through existing interfaces in MCC modules.
  Keep MCC compatibility logic in MCC-owned modules and shared additions
  as small as possible. Guard MCC-specific execution and state so Xbox and
  Custom Edition retain their existing paths, behavior and limits. Handle
  MCC discovery before loading and cleanup after unloading explicitly.
- For each shared addition, explain why an MCC-module-only solution is
  insufficient, identify the affected engine paths, mark source additions
  with `port:` as required below, and verify legacy behavior and map-family
  transitions. This permission covers necessary integration, not replacing,
  renaming or refactoring established Xbox or Custom Edition implementations
  for convenience or code reduction. Existing hooks are not blanket
  permission for broader changes. See `CODINGSTANDARDS.md` for the MCC
  boundary inventory. If a feature cannot fit within these constraints,
  explain the specific conflict and stop that part of the work. Do not
  proceed unless the user explicitly changes the applicable constraint.
- The general permission below for minimal changes to `source/` does not
  override these stricter rules for MCC work. Refactoring, deduplication,
  cleanup and line-count targets are not exceptions.
- Explicitly requested upstream merges and general engine fixes have their
  own scope, as described below. They do not authorize unrelated changes
  to Xbox or Custom Edition support or relax the rules for implementing
  MCC compatibility.
- Check preservation of Xbox and Custom Edition behavior as well as MCC
  compatibility. Report the actual validation performed and its limits;
  a successful load or green test suite is not proof of full compatibility.

### Existing behavior fixes and upstream merges

- Authorized fixes to existing behavior may modify the responsible
  implementation directly; they do not require a separate feature module
  merely because code changes are needed. Keep the fix focused, explain
  the intended behavior change, and preserve unrelated functionality.
  Retain the required `port:` markers for changes to `source/`.
- MCC compatibility fixes remain subject to MCC isolation. Calling work
  a bug fix or general engine fix does not authorize repurposing Xbox or
  Custom Edition implementations for MCC.
- Base the distinction between a general engine defect and an MCC
  compatibility requirement on evidence about the existing engine contract,
  accepted inputs and intended behavior. An MCC map exposing a failure does
  not by itself establish either classification. Document why a proposed
  shared fix corrects existing engine behavior rather than adapting it to
  MCC-specific data or semantics. If the distinction remains unresolved and
  affects whether a change is permitted, explain the uncertainty and stop
  that part of the work.
- For explicitly requested upstream merges, retain authoritative OpenCE
  changes and preserve MCC and other feature integration. Do not use the
  merge to introduce unrelated changes to Xbox or Custom Edition support.
  Identify and validate behavior affected by conflict resolution.
- Apply the required platform, networking and regression checks to the
  affected paths. Report what was verified and any remaining limits.

### Project preferences

- Use descriptive branch names without a `codex/` prefix.
- Do not write or update PR descriptions unless the user explicitly asks;
  the user handles them. This does not remove the requirement to record
  rationale and validation in commit messages.
- Run at most two game instances concurrently during automated testing.
  A two-instance co-op test within an authorized task needs no additional
  permission. Use isolated data/save folders and hidden windows or a
  virtual display; stop only the processes you started.
- Keep temporary test maps bounded and clean up only your own verified
  fixtures after testing. Preserve the user's original maps, saves and
  useful diagnostic logs.

## Layout

| Path | Contents |
| --- | --- |
| `source/` | The game: 466 C files of the decompilation, compiled as they are with few changes (each marked, below) |
| `port/linux/src/` | The platform layer the three ports share: Direct3D 8 on OpenGL (`d3d8_gl.c`), DirectSound and XInput on SDL3, files, threads, memory, sockets, settings (`port_config.c`), internet play (`p2p*.c`) |
| `port/linux/game/` | The port's game-side code, compiled as the game's sources are: the distributed netcode (`network_*.c`), co-op, map sharing (`map_share*.c`), the menus (`menu_tags.c`, `menu_functions.c`), Custom Edition maps (`custom_edition_*.c`, `cache_file_formats.c`), the tag validator (`tag_validate.c`, `tag_schema_*.c`), touch, FOV and more |
| `port/linux/include/` | The port's headers: the limits (`halo_port_limits.h`, with `HALO_PORT_NETWORK_VERSION`), capacities (`halo_port_capacity.h`), the prefix header, `halo_linux_source_fixups.h` (port functions the game's sources call) |
| `port/include/xdk/` | Stand-ins for the Xbox SDK's headers (declarations only) |
| `port/windows/` | The Windows build's own files (`win32_*.c`, headers) |
| `port/android/` | The Android app: the guest runtime, the host library (`host/`), the Java app (`app/`), host imports (`host_imports.list`) |
| `port/web/` | The web build: its runtime (`src/`: entry, sockets inside the page, memory watch), the page and its service worker (`site/`), the SDL port, tests |
| `port/assets/` | What the builds embed or ship: high-res HUD (`hud/`), fonts, titles, menus (`menus/`), icons, network brokers |
| `port/third_party/` | Vendored libraries, each with a README naming its upstream, version and checksum |
| `port/tools/` | Stand-alone tools (`cache_file_report.c`) |
| `tools/` | Build scripts (`linux_build.py`, `windows_build.py`, `android_build.py`, `web_build.py`, `ci_build.py`, `web_serve.py`), generators (`ce_menus.py`, `port_settings.py`, `hud_assets.py`, `title_assets.py`, `embed_assets.py`, `xdk_headers.py`) and tests |
| `docs/` | Longer design notes (`custom_edition_caches.md`) |
| `pgo/` | Profile-guided optimisation profiles |
| `assets/` | Game data for local runs (gitignored: `maps/`, `custom_maps/`) |
| `.github/workflows/` | CI: builds every platform on each push, publishes releases from `main` |

## Building

```sh
python configure.py            # debug build; add --release, --portable, --profile, --pgo=off, --lto=off
ninja linux                    # build/linux/halo
ninja android                  # the Android guest image and native libraries
ninja android_apk              # the APK (Gradle)
ninja web                      # build/web/site (Emscripten: port/web/README.md)
ninja windows                  # on Windows only
python tools/ci_build.py linux release   # what CI builds
```

- `configure.py` writes `build.ninja`; `configure_args` at its top records
  the options it was run with. Restore them after trying other options.
- The Linux and Windows builds are 32-bit x86 with clang, in MSVC's ABI
  (`-fms-extensions -fshort-wchar -malign-double -fcommon`), C89 (`gnu89`):
  declarations come before statements.
- Every native build compiles without fused multiply-add and with the
  vendored musl maths (`halo_math.h`), so that every machine of a network
  game computes the same results. Do not call the system's maths functions.
- Android: the game is an ILP32 AArch64 guest (`arm64_32`) in a 64-bit app.
  Guest code can call only the host imports declared in
  `port/android/host_imports.list` and `port/android/guest/runtime/guest_host.h`,
  and only the SDL functions `guest_sdl.c` bridges. Run `ninja android`
  after changing anything it compiles: `port/linux/src`, `port/linux/game`,
  `port/android`, `source/`, the build scripts.
- Web: the game is `wasm32` (also ILP32), run on a Web Worker; calls must
  match their callee's type exactly (the link fails on a mismatch), and
  WebGL 2 is ES 3.0. Run `ninja web` after changing what it compiles (as
  for Android, and `port/web`). Refer to [port/web/README.md](port/web/README.md).

### Defines

| Define | Set by | Meaning |
| --- | --- | --- |
| `HALO_ANDROID` | Android | The app: its display, input, files and lifecycle |
| `HALO_GLES` | Android, web | The OpenGL ES renderer |
| `HALO_ARM64_GUEST` | Android | The guest's ABI (ILP32 AArch64) |
| `HALO_WEB` | Web | The browser: its frames, storage, sockets and WebGL 2's limits (with `HALO_GLES`, not `HALO_ANDROID`) |
| `HALO_WINDOWS` | Windows (`halo_windows_prefix.h`) | The Windows build |
| `HALO_RELEASE` | `--release` | No assertions checked |
| `HALO_PROFILE` | `--profile` | The profiling build; a normal build must be unchanged by it |

`HALO_LINUX` and `HALO_CUSTOM_EDITION` are **not** defined by any build:
code guarded by them compiles to nothing. Custom Edition support is a
setting (`game.custom_edition`), checked at run time with
`custom_edition_cache_tags_loaded()`.

## Testing

```sh
python -m pytest -q tools/harness tools/test_touch_menu.py   # the engine's functions in a fake world
python -m pytest -q tools/test_linux_port.py                # the build, the maps' checks (needs assets/maps)
python -m pytest -q tools/test_cache_file_formats.py tools/test_bmp_files.py tools/test_profile.py
python tools/test_touch_layout.py                           # the Android touch layout (JDK 17+)
python tools/test_light_storage.py
python tools/test_death_timing.py
python -m pytest -q tools/test_web_build.py                 # the web build's graph, sockets, menus
node --test "port/web/tests/*.test.js"                      # the web page's importer and service worker
node port/web/tests/webrtc_native.mjs chromium firefox      # the native builds' WebRTC with browsers (Playwright)
```

`tools/harness/README.md` explains how to add an asset-free test: the code
under test is taken from the sources, not copied.

Running the game without a person at it:

- `debug.network_test` (`HALO_NETWORK_TEST=host:<map>` or `join`) hosts or
  joins a game without the menus and logs each player's state every second;
  several copies play on one machine on different loopback addresses
  (`HALO_NET_ADDRESS`, `HALO_NET_BROADCAST`). `NETCODE.md` describes the
  test settings.
- `debug.exit_after`, `debug.hidden_window`, `debug.null_renderer`,
  `debug.menu_open` (start on one menu screen), `debug.telnet_console`.
- Use a virtual display (Xvfb, with `SDL_VIDEO_DRIVER=x11`) for windowed
  runs, never the desktop of the person you work for, and stop only the
  processes you started.

## Rules of the project

### The game's sources

- Change `source/` as little as possible, and mark each change from the
  original with a comment that starts `port:`. Write like the surrounding
  code: its naming, its comment density, and the project's plain,
  descriptive comment style.
- A port function the game's sources call is declared in
  `halo_linux_source_fixups.h` or included with a note naming its file
  (`#include "view_fov.h" /* port: port/linux/game/view_fov.c */`).
- Notes such as `BUG (original, preserved for exact matching)` mark the
  original's bugs. Fixing one is allowed; say so in a `port:` comment.

### The Xbox SDK

`port/include/xdk` was written without the SDK's headers, from the January
build's debug information and public documentation. Never read the SDK's
headers or anything derived from them, never copy from them, and do not
read `xbox/include`. To add a name, follow `port/include/xdk/README.md`.

### Networking

- Raise `HALO_PORT_NETWORK_VERSION` (`port/linux/include/halo_port_limits.h`)
  with any change to what the machines send each other, or to anything they
  must agree on, and say why in the commit. Machines of different versions
  do not play together.
- Only the host decides; clients predict their own players. Read
  `port/linux/NETCODE.md` before changing the netcode.
- Internet play is the same code in every build, the web's too: a browser's
  tunnel is a WebRTC data channel (`port/web/src/web_p2p.c`), which the
  native builds take on their tunnel's socket (`p2p_webrtc.c`, `posix_dtls.c`).
  Check a change with both (`port/web/tests/webrtc_native.mjs`, and with the
  maps `port/web/tests/internet.mjs`).
- Anything another machine sends is untrusted: check sizes, indices and
  rates, as the existing handlers do. `debug.network_corrupt` tests this.

### Menus

- The menus must match Halo Custom Edition's. Their XML (`port/assets/menus/ce/`)
  is generated by `tools/ce_menus.py` from the PC version's tags; the
  settings screens come from `tools/port_settings.py`. Do not edit the XML
  by hand: change the generator and run it (the command is in
  `port/assets/menus/README.md`, "Writing them again"), then check that
  `git status` shows only the screens meant to change.
- Functions the menus call but the port has not wired are listed in
  `port/assets/menus/UNWIRED.md`.

### Art and credit

- Replacement HUD, title and menu art is made of hand-drawn redraws.
- Prefer an existing redraw (the ui-svg-handmade set), then a new redraw. A
  Custom Edition picture that cannot reasonably be redrawn (a 3D render, a
  screenshot, a logo) may be used only if it is listed in
  `port/assets/menus/NON_HANDDRAWN.md`.
- `AUTHORS.md` credits art only.
- Fonts: Overpass and OpenCE (Newtown, respaced) only; see
  `port/assets/fonts/README.md`. Never ship the maps' commercial typefaces.

### Custom Edition maps

Custom Edition maps live in the data root's `custom_maps` folder with
`bitmaps.map`, `sounds.map` and `loc.map`; their level names are
`custom_maps\<name>`. OpenSauce features are not supported and must not be
added: a map that needs them is refused, and one that only carries
OpenSauce's header runs as stock Custom Edition runs it. Map files are
untrusted input: read them through bounds-checked readers. See
`docs/custom_edition_caches.md`.

### Settings

A setting is an entry in `port/linux/src/port_config.c`, with a default, an
environment variable and a comment that `config.toml` gets. Document it in
the settings table of `port/linux/README.md` (and the Android README for an
Android-only one). A departure from the stock game's look or behaviour
should be a setting, off or stock by default, unless the project decides
otherwise.

### Third-party code and licences

The repository is CC0. Vendored code goes in `port/third_party/<name>`,
copied unchanged, with a README naming the upstream, the version or commit,
the checksum and what uses it. GPL code (OpenSauce, Reclaimer) may be read
as documentation of file formats, never copied.

### Commits and pull requests

- Commit subjects are plain sentences saying what changed; the body says
  why and what was checked.
- A pull request is merged with a merge commit (`Merge pull request #N from
  <owner>/<branch>`), and follow-up changes go in separate commits whose
  subject ends with `(#N)`.
- A commit taken from a fork keeps its author and is made with
  `git cherry-pick -x`.

## Documents

| Document | About |
| --- | --- |
| [README.md](README.md) | Downloads, game data, building, build options, profiling builds |
| [CODINGSTANDARDS.md](CODINGSTANDARDS.md) | Supplementary coding, MCC boundaries, evidence and review standards |
| [port/linux/README.md](port/linux/README.md) | The Linux build, controls, menus, every setting, updates, frame rate, field of view, system link, internet play, voice chat, the server browser, map sharing, security, map checks, how the port operates, the game source changes |
| [port/linux/NETCODE.md](port/linux/NETCODE.md) | The distributed netcode, network versions, joining a game in progress, map sharing, transport, testing |
| [port/windows/README.md](port/windows/README.md) | The Windows build, its headers, inline functions, crash reports |
| [port/android/README.md](port/android/README.md) | The Android build, the guest and host, touch controls, Android settings, finding problems |
| [port/web/README.md](port/web/README.md) | The web build, the page, the game data in the browser, limits, how it operates, hosting on GitHub Pages |
| [port/include/xdk/README.md](port/include/xdk/README.md) | How the SDK declarations were written and are checked |
| [docs/custom_edition_caches.md](docs/custom_edition_caches.md) | Loading and converting Custom Edition maps, and the evidence for each layout |
| [port/assets/menus/README.md](port/assets/menus/README.md) | The menu files' format and how to write them again |
| [port/assets/menus/NON_HANDDRAWN.md](port/assets/menus/NON_HANDDRAWN.md) | Menu pictures that are not redraws |
| [port/assets/menus/UNWIRED.md](port/assets/menus/UNWIRED.md) | Menu functions that do nothing yet |
| [port/assets/fonts/README.md](port/assets/fonts/README.md) | The fonts and their licences |
| [tools/harness/README.md](tools/harness/README.md) | Asset-free regression tests |
| `port/third_party/*/README.md` | Each vendored library |
| [AUTHORS.md](AUTHORS.md) | Art credits |
