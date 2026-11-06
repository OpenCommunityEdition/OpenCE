# Dynamic lighting 1.0

The GulchCE dynamic lighting 1.0 baseline was accepted by the project owner on
2026-10-27 and promoted from `dynamic-lighting-test` through `testing` to `main`. The known
performance and memory limitations below remain; acceptance does not establish
sustained 120 FPS or universal map compatibility.
CStencil and Modern shadows remain separate, later work.

## Disabled desktop shader isolation

Desktop and web model draws now select stock shader variants whenever no
optional model light is selected. This removes optional shader helpers and
samplers from those variants and skips their GPU setup. Enhanced variants are
compiled lazily, cached separately and selected using the existing effective
light mask, including host-controlled Shield Glow. Defaults remain off.

This closes a gap in the previous disabled-path checks: no submitted feature
lights did not establish that the underlying shaders and sampler setup were
stock. Previously recorded disabled performance results used the extended
shaders and must not be treated as measurements of this corrected path.
No FPS or memory improvement is claimed without a new measured comparison.
First-use shader hitches and the unresolved sustained-performance/memory
limitations below remain; used shader variants retain context-lifetime caches.

See the [rendering guide](reading/rendering.md#dynamic-lighting-10) for variant
ownership and the unchanged Android boundary.

The focused Windows validation passed 125 tests covering shader variants,
source-cache reuse, light lifecycle, occlusion, host policy, Android exclusion
and MCC shader boundaries. Generated model GLSL was compared with the prior
revision for 67 real vertex programs, packed/unpacked inputs and per-vertex/
per-pixel modes: enabled output was byte-identical, and disabled output matched
the prior translator with its optional additions excluded. Desktop, web and
Android preprocessing were checked separately.

Six short rendered Blood Gulch checks at 1280x720 passed at 30, 60 and 120 FPS,
with lighting on and off. Disabled runs selected no optional lights and made
no occlusion updates; enabled runs reached enhanced model draws. Captures were
inspected on an NVIDIA RTX 5070 Ti. These checks are not new sustained-performance
measurements or AMD hardware coverage. The local Windows portable release
build passed; Android/web targets are unavailable in this local toolchain and
require the platform CI builds.

The rendered two-peer Xbox check observed joining, weapon changes, two respawns
and host-forced Shield Glow on a client with its local setting off. All 140
stable policy samples agreed with the host, with no shader errors or reported
state divergences. Its strict paired-snapshot verdict remained incomplete:
the once-per-second logs were offset by more than the comparator's two-tick
window, so zero snapshots were paired. The Custom Edition Death Island check
passed the full scenario, including 80 seconds of paired snapshots and host
stock-shield policy overriding the client's setting. MCC `DigsiteMP/1` also
passed, with 64 seconds paired, respawns, weapon changes and independent local
lighting under the host's `players` policy. Neither scenario reported shader
errors or snapshot divergences. Initial sandboxed attempts
could not establish local sockets and are excluded from gameplay validation;
the working checks ran outside that sandbox, with internet publishing disabled.

## Android compatibility fallback

Following reports of missing first-person geometry and dark models on multiple
Android device families, Android builds exclude the optional lighting modules,
material glow overrides, model-shader additions and extra sampler bindings.
The supplied clip shows these symptoms on a Custom Edition map; it does not
establish the underlying driver error. Source inspection found that the added
model shader code and sampler setup were present even with all toggles off.

Android now links inert feature hooks that preserve stock light values and the
overshield surface reset. The Dynamic Lighting menu and its default config keys
are unavailable there; old saved values cannot enable the omitted code. Stock
lighting and effects remain, as do Animated Overshield surfaces. Network policy
and messages are unchanged: Android's lack of Shield Glow rendering does not
change the host's choice for other clients. Desktop and web implementations,
AMD/NVIDIA workarounds and Xbox/CE/MCC loading boundaries are unchanged.

See [Android lighting availability](../port/android/README.md#lighting-availability)
for the build boundary. Device testing is required before claiming the reported
rendering failures are resolved; build and harness checks alone cannot do that.

## Promotion to testing

The 1.0 promotion combines `testing` at `87eb88e2` with the complete
`dynamic-lighting-test` history through `2e21f098`. It retains testing's
controller remapping, automatic button styles, player-only shield looks,
GPU frame pacing and desktop program-binary cache. The feature modules and
AMD/NVIDIA shader translation fixes are unchanged from the accepted branch.

Two merge conflicts needed integration: shared fragment shaders now register
their source hash once with the upstream disk cache, and menu generation
retains both the lighting screen's help height and dynamic controller help.
The real-source cache harness checks hash registration, including a negative
mutation; generated-menu checks cover both help-widget contracts. No new
feature hook, network message, default or host-policy rule was introduced.

The merged Windows portable release build passed. Focused lighting, menu,
controller, policy, map-boundary and network-comparator checks passed: 297
tests, with nine skips (five POSIX process cases and four optional map-reference
checks whose local inputs were absent). Ordinary/full-LTO light-storage checks
and their corruption negative control also passed. Menu regeneration produced
only the intended lighting screens.

Three hidden 22-second native runs loaded Xbox c40 at 1280x720/60 FPS. The
enabled cold run wrote 49 program binaries; the enabled warm run loaded all
49, and the disabled run loaded 48. Warm/off runs linked no new programs;
none reported rejected binaries, shader errors or a crash. Captures show the
opening cinematic rendering normally. This is a cache integration smoke
check, not a new combat or performance acceptance run. An initial local-game
fixture remained in menus and was rejected; the corrected fixture uses a
copied profile and direct map load. Both attempts retain their diagnostic logs.

A hidden enabled transition check loaded Xbox c40, MCC Mercury Falling, CE
Death Island and Xbox Blood Gulch in one process. All three transitions reset
the simulation tick as expected; shield activation remained enabled, with no
exception or forced shutdown. The passive probe was then removed and the
normal build restored. Staged maps and generated map caches were removed;
test saves were archived and verified before removing their loose copies.

The existing visual, multiplayer and host-policy evidence below remains
applicable to unchanged feature code. Local Android/web toolchains remain
unavailable; the testing push runs the platform and MCC workflows. Known
performance and memory limitations remain documented below.

The first MCC CI run exposed an existing mock-header defect in testing's
texture-bridge harness: both the fixture and included bridge pull in its
unguarded `zlib.h`. Linux C99 rejects the duplicate `uInt` typedef under
`-Werror`; the Windows compiler accepted it. A follow-up adds the missing
mock-header guard without changing runtime code or suppressing diagnostics.
All ten texture-bridge cases and fifteen fragment-cache cases passed locally
after that correction; CI supplies the Linux confirmation.

## Promotion to main

The main promotion merges all of `testing` through `a83584c6` into main at
`08baa774`. The merge is conflict-free and its implementation is identical
to testing; only documentation differs, retaining main's release-checksum
instructions and recording availability on main. Dynamic lighting remains
off by default, with the existing host policy and compatibility boundaries.
Controller, HUD and upstream renderer updates already integrated on testing
are included in the same promotion. The main workflow builds and validates
the release packages; the known limitations below remain applicable.

The final testing CI passed 1,757 engine/harness cases before exposing three
controller-artwork regeneration failures on Pillow 13. Its new RGBA thumbnail
reduction pass differed from the committed Pillow 12.3 output. The follow-up
sets `reducing_gap=None` in the artwork generators; all 28 controller/HUD
checks pass with both Pillow versions, preserving exact committed pixels.
No shipped artwork or runtime code changed. The main promotion also passed
174 changelog/release-note checks and generated its required player notes.

## Player controls

Open **Settings > Video Setup > Dynamic Lighting**. All lighting switches
default to **Off**; Glow Intensity defaults to **Default**. Saving applies the
pending choices, cancelling discards them, and Defaults restores the stock
choices for this screen. Saved choices persist across launches.

| Menu row | What it controls |
| --- | --- |
| Projectile Lights | Existing plasma, rocket and Covenant projectile profiles, armed plasma grenades and flamethrower streams. |
| Muzzle Flash Lights | Extra short-lived weapon illumination, including the Warthog turret and both Scorpion guns; stock flash and explosion effects remain intact. |
| Weapon Glow | Energy swords, plasma charge/heat, Grunt/Hunter fuel rods, AR counters and Needler crystals. |
| Shield Emitters | Blue/orange Jackal shields and Covenant stationary shields, including damage colours. |
| Vehicle Lights | Supported Covenant running/engine glow and directional Warthog/Scorpion headlights. |
| World Emitters | Overshield/active-camouflage pickups and visible green teleporter surfaces. |
| Light Occlusion | Optional blocking for projectiles and emitters. Muzzle and vehicle lights always request collision blocking, including their chassis. |
| Shield Glow | Player/unit shield spill, subject to the host's shield-effects policy. |
| Glow Intensity | Default or Light Show intensity for Shield Glow; this does not enable it. |

Weapon Glow and Shield Emitters consolidate the old per-effect menu rows.
Both rows offer only OFF and ON. Older saved dynamic-lighting preferences
reset to OFF once; later player choices persist. There is no CUSTOM state.
See the [emitter expansion walkthrough](reading/emitter_expansion.md)
for exact scope, lifecycle, budgets and map-asset limits. The historical 1.0
acceptance results below predate these additions; they are not evidence for
the expansion's runtime acceptance.

**Animated Overshield** remains in **Video Setup > Extra Effects**, also off
by default. First-person shield surfaces additionally depend on **Arm
Shields**. In multiplayer the host's **Shield Effects** policy remains
authoritative; **Players Choose** permits each player's shield preferences.
Other lighting choices are local presentation settings and do not change
damage, collisions or network simulation. A stock host policy can override
the local shield toggles; that is intentional.

Frame interpolation is a separate setting and supplies smooth poses between
simulation ticks. Existing per-pixel lighting is also separate. Neither is
silently enabled by the lighting submenu. Custom maps must use a supported
tag/profile; this feature does not infer arbitrary modded weapon semantics.
Xbox, CE and MCC retain their separate loading paths.

## Implementation and final review

Start with the [rendering walkthrough](reading/rendering.md) and
[shield walkthrough](reading/shield_lighting.md). The
[implementation history](plasma_lighting.md) preserves earlier decisions and
validation; its initial prototype restrictions are historical. The
[emitter notes](covenant_emitter_lighting.md),
[compatibility report](dynamic_lighting_compatibility.md) and
[stability investigation](dynamic_lighting_stability.md) contain detailed
evidence and its limits.

The Milestone 5 review follows the combined projectile, muzzle, held-emitter,
shield and overshield paths through settings, host policy, temporary light
ownership, rendering hooks, occlusion, shader generation and lifecycle
cleanup. Capture and profiling fixtures remain in tools, outside normal
builds. Bounded storage, salted object identities, authored-light precedence,
platform stubs, collision checks and shader-cache source comparisons are
necessary protections and were retained.

Two feature-local simplifications have a concrete benefit:

- `covenant_visible_profile` in
  [covenant_lighting.c](../port/linux/game/covenant_lighting.c) returns its
  validated profile to the caller. Admission no longer repeats object/tag
  lookup after validation. With ten eligible projectiles and eight light
  slots, the harness observes ten tag queries on initial admission instead
  of twenty-six, and eighteen on a retained update instead of twenty-six.
  The visibility/ownership validation is preserved; profile zero remains
  valid. These are lookup counts, not a measured FPS improvement.
- `overshield_effect_find_values` in
  [overshield_effect.c](../port/linux/game/overshield_effect.c) shares the
  existing active body/arms lookup. Colour selection no longer scans the
  effect array twice or computes unused fade geometry. Activation, host
  policy, charge timing, fade equations and colours remain unchanged.

No shared engine, network protocol, map loader, AMD/NVIDIA workaround or
shader-cache behaviour is changed by these final refactors. Existing shared
hooks remain necessary because feature modules cannot independently intercept
the engine's light submission, interpolated model draw and frame cleanup.
This pass adds no shared hook and does not merge map-family implementations.

## Known limitations

- Sustained 120 FPS is not established. The earlier eight-minute enabled
  busy-combat run at native 2560x1440 averaged approximately 115.98 FPS at a
  120 cap. Short captures at 30/60/120 FPS are visual checks, not endurance
  certification.
- Shader compilation/program linking can still hitch. A capture-free,
  lighting-disabled run recorded a 51.229 ms frame containing 10.977 ms of
  shader compilation and 36.805 ms of program linking. The earlier 412/221 ms
  stalls were not reproduced, and their cause remains unresolved.
- Memory has not demonstrated a plateau. The enabled capture-free run grew
  approximately 3.64 MiB between early/late windows, with a later fitted
  trend of 1.52 MiB/minute. The disabled control also had a late approximately
  27 MiB jump near shader compilation. Allocation ownership is unresolved;
  these observations neither prove a leak nor establish stability. The
  completed shader-cache deduplication does not explain all remaining growth.
- Runtime evidence is representative, not exhaustive: Windows/NVIDIA runs
  do not certify AMD hardware, all drivers, every custom map, internet play
  or Android/web visual parity. Platform builds and targeted regression tests
  establish narrower facts. See the linked reports for actual checks.

The final optimisations above do not claim to fix these limitations. The project
owner's playtest acceptance completes the 1.0 sign-off with these limits documented.

## Milestone 5 validation and delivery

The final functional changes are in `7b1e8c02`; `152142a4` adds documentation
and corrects an overshield menu-location comment. The Windows playtest uses
`152142a4`, built with `--portable --release --lto=off --pgo=off`.

| Check | Observed result |
| --- | --- |
| Focused local regressions | 205 passed; five POSIX process/launcher cases skipped on Windows. Includes ownership, off/reset paths, falloff/occlusion, shield policy and body/arms effects, fragment-cache and visibility contracts, settings and MCC networking. |
| Touch-menu unit | Passed separately with the installed Windows toolchain's explicit x86 target and CRT link flags. The generic pytest invocation did not supply those flags; no touch implementation change was needed. |
| Short rendered captures | Xbox Needler at 30/120 FPS, CE Needler at 60 FPS, and Xbox overshield charge at 60 FPS with Shield Glow on/off all passed the runner's checks. |
| Mixed-setting peers | Two hidden Windows instances at 1280x720/60 FPS, Xbox Blood Gulch, with opposite local lighting choices and a host-forced shield policy. Both observed two respawns of the same player and inventory changes. All 139 stable policy samples matched; 81 seconds of paired snapshots had no comparator divergence. |
| Map lifecycle | One enabled Windows process loaded Xbox c40, MCC Mercury Falling, CE Death Island and Xbox Blood Gulch, with three tick resets, expected shield activation and no exception. |
| Platform CI | [Windows, Linux, Android and web passed](https://github.com/chestahh/GulchCE/actions/runs/40266569136), including Linux tests, browser/native WebRTC checks and Chromium/Firefox web checks. [MCC validation also passed](https://github.com/chestahh/GulchCE/actions/runs/40266569124). |
| Delivered ZIP | Archive integrity and every packaged file verified; extracted executable started and displayed the Dynamic Lighting menu. All nine lighting booleans plus Overshield remained false, intensity Default and host policy Stock. No diagnostic fixture strings were present. |

The five short visual scenes use 640x480 isolated captures. Frame-log averages
were 29.19/119.99 FPS for Xbox Needler, 59.96 for CE Needler and 58.80/60.00
for shield spill on/off; all met the runner's five-percent tolerance. Image
inspection showed purple projectile spill, the retained overshield charge
rise and green-to-pink layer transition. With Shield Glow off, the independently
enabled overshield surface remained and the added spill disappeared. These
are scoped visual observations, not sustained performance measurements or
exhaustive aesthetic acceptance. The capture executable predates only the
menu-location comment; source hashes and that exact comment-only difference
were checked. The multiplayer/lifecycle probe and normal build use `152142a4`.

Local Android/web targets are unavailable in this Windows configuration;
fresh CI supplies those build checks. CI uses debug builds on this branch,
while the local playtest is release. There was no new AMD hardware test or
retail internet game. The broader Milestone 4 map/policy matrix and prior
menu interaction evidence remain in the linked reports; this pass did not
repeat that entire matrix. The initial test selection exposed a harness
extraction dependency on the new overshield helper, which was corrected
before the final passing suite. No assertion or compatibility guard was
removed to obtain a pass.

Review also confirmed that the final changes leave `source/`, platform
renderer code, map loaders, host-policy code and network version untouched.
The normal build was restored after each diagnostic build. No new shared
hook, gameplay mutation, forced policy or diagnostic is shipped. The review
retained necessary bounded caches, checks and platform stubs instead of
removing them solely to reduce code size. Fifty-three local documentation
links and the newly named functions were checked.

Delivery is `GulchCE-dynamic-lighting-windows-152142a4.zip` under the primary
checkout's `build/playtest/`, with an adjacent `-PLAYTEST.txt` and `.sha256`.
The 13.4 MiB ZIP includes its own `PLAYTEST.txt`, stock configuration,
`BUILD.json`, source download links and licence notices; it contains no maps
or saves. Its SHA-256 is
`bce57d6b01ef3733c9f374257ba49070f1306eef04070d9e055cc0ecd724ff06`.

Evidence remains under the active checkout's `build/milestone5-*`,
`build/plasma-visual/*-m5-*` and `build/lighting-compatibility/m5-*`.
All owned processes exited. Runners removed staged maps/caches; final cleanup
removed approximately 60.6 MiB of test binaries/objects and 186.7 MiB of
redundant capture frames. Representative projectile images, all shield/menu/
network/transition captures, complete logs and manifests remain. All 167
archived test-save files (180.1 MiB uncompressed) passed SHA-256 verification.
Approximately 50.3 MiB of diagnostic evidence remains, besides the delivery
ZIP and required normal Windows build/symbols. Original maps, saves,
toolchains, other worktrees and earlier milestone evidence were preserved.

## Scorpion, Hunter and first-person AR additions (2026-10-27)

Implementation and Windows playtest revision: `2522c432`. These additions use
the existing Muzzle Flash Lights, Fuel Rod Glow and AR Counter Glow options.
All remain off by default. See the [muzzle notes](muzzle_lighting.md) and
[emitter notes](covenant_emitter_lighting.md) for tag/marker evidence and tuning.
The existing performance and memory limitations above remain unresolved.

Self-review covered the combined feature modules, seven marked shared draw
lines, harnesses, capture fixtures, generated menu help and documentation.
Scorpion triggers share the existing bounded pool but retain independent
pulses. A profile flag replaces the repeated Warthog-only occlusion name test.
The Hunter reuses the fuel-rod profile through its animated biped marker;
its model-less weapon remains excluded. The AR material lookup is shared by
its two helpers, and the exact output state is restored after the display
draw. World-model values and shared fade history are unchanged. No loader,
network protocol, host policy, shader translator or AMD/NVIDIA workaround
was modified. Broader renderer refactoring was unnecessary for this scope.

Sixteen hidden Windows captures on an NVIDIA RTX 5070 Ti, driver 617.42,
used isolated data at 640x480, per-pixel lighting and the requested frame caps.
The three new cases passed at every cap; observed mean FPS was:

| Case | 30 cap | 60 cap | 120 cap |
| --- | ---: | ---: | ---: |
| Scorpion, both guns | 30.00 | 59.99 | 120.04 |
| Hunter rods, Xbox c40 | 30.00 | 60.02 | 119.99 |
| First-person AR, Xbox Blood Gulch | 30.00 | 60.05 | 120.02 |

The remaining seven cases were 60-FPS off controls for each addition,
Warthog and third-person AR regressions, and CE Death Island AR/Scorpion
captures. Off controls selected no added lights. Images inspected at all
three rates showed warm Scorpion flashes with a dark underside in the tested
orientations, green Hunter forearm spill and a brighter readable AR screen.
Both Scorpion triggers submitted independent pulses; Hunter captures reached
model diffuse and specular draws. These are short visual checks, not sustained
performance measurements, exhaustive occlusion coverage or new AMD validation.

The focused regression suite passed 206 tests, with five POSIX-only cases
skipped on Windows. COFF/full-LTO light storage and the standalone touch-menu
unit passed. The generic touch/web pytest run hit Windows CRT/link and path
assumptions; the touch unit passed with explicit x86/CRT flags. Local Android
and web build targets remain unavailable, so CI supplies platform validation.

Two-peer rendered checks passed on Xbox Blood Gulch, CE Death Island and
MCC DigsiteMP with opposite local lighting choices. Host policy matched all
386 stable samples, both peers observed respawns and inventory changes, and
the existing comparator accepted 81/81/64 seconds of paired snapshots.
One enabled process then loaded Xbox c40, MCC Mercury Falling, CE Death Island
and Xbox Blood Gulch: three tick resets, expected shield activation, clean
exit and no logged exception. This checks representative transitions, not
campaign completion or every custom model reusing a stock tag name.

[Windows, Linux, Android and web CI passed](https://github.com/chestahh/GulchCE/actions/runs/40432901781),
including debug builds, Linux tests, browser/native WebRTC and
Chromium/Firefox web checks. [MCC validation also passed](https://github.com/chestahh/GulchCE/actions/runs/40432901683).
CI skips release builds on this branch; the Windows playtest was built locally
in release mode.
These runs validate the implementation revision `2522c432`; this appended
acceptance record changes documentation only.

The normal release ZIP uses `--portable --release --lto=off --pgo=off` and
contains no game maps, saves, symbols or capture fixtures. Its extracted
executable started, displayed the Dynamic Lighting menu, and retained all nine
lighting switches plus Overshield off, Default intensity and Stock host policy.
Archive integrity and every packaged file were verified. The ZIP SHA-256 is
`8650c86a10a64967f29193308445e96cd2f982aa895fa5231db57a16831d449d`.

Evidence is under `build/lighting-extension-*`,
`build/plasma-visual/*-extension-*` and `build/lighting-compatibility/extension-*`.
Capture manifests identify the working-tree sources before commit; their only
later source differences are two explanatory comments and the Fuel Rod Glow
configuration description. Runners removed staged maps and caches and archived
test saves with hash verification. Final cleanup removed another 220.9 MiB of
diagnostic executables and redundant images, retaining sixteen representative
images per capture case, full logs, manifests and save archives. The per-run
evidence totals about 107.0 MiB, besides contact sheets, small build logs, the
13.6 MiB ZIP and the required normal build/symbols. Original game data, saves
and earlier evidence remain untouched; all test processes exited.


## AR world display and fuel-rod colour tuning (2026-10-27)

Implementation revision: `f76fc69c`; comparison baseline: `9cac0595` (the
previous `2522c432` implementation plus its acceptance documentation). The
existing AR Counter Glow and Projectile Lights options remain off by default.
The world-model display now shares the first-person emissive material boost.
Its spill module is byte-for-byte identical between the capture builds;
recorded steady world-light RGB remains (0.156, 0.4225, 0.65).

The exact shared Grunt/Hunter projectile exhaust gets less red and blue in
its render submission, retaining the authored green channel and modulation.
Read-only Xbox c40 trigger references establish that both weapons use that
projectile. Runtime tests below exercise this shared projectile definition,
not separate AI firing sessions for each weapon. The source light datum,
stock flare, trail, impact and explosion remain unchanged. See the
[emitter](covenant_emitter_lighting.md) and
[projectile](covenant_lighting.md) notes for policy and tag evidence.

The combined review covered both feature modules, declarations, shared hooks,
fixtures, tests and documentation. It removed the redundant material-helper
view argument, kept AR spill and renderer translation untouched, and added
only a four-line marked hook for the private render-colour copy. The new
map-scoped tag cache uses existing reset boundaries. No loader, host policy,
network contract or AMD/NVIDIA workaround changed. The existing performance,
memory and approximate-occlusion limitations remain.

Focused regressions passed: 207 tests, with five POSIX-only launcher/process
cases skipped on Windows. Ordinary and full-LTO light-storage checks passed,
including their deliberate pointer-corruption negative control. The new colour
case covers off, invalid owners, unrelated tags/lights, modulation, missing tags
and map resets. Existing emitter coverage retains exact third-person/first-person
spill gains and checks material-state restoration.

Hidden, serial Windows captures used 1280x720, an NVIDIA RTX 5070 Ti (617.42),
per-pixel lighting and Light Occlusion. Both old and new enabled tuning were
captured at each cap and visually reviewed. Measured short-capture means:

| Case | 30 cap | 60 cap | 120 cap |
| --- | ---: | ---: | ---: |
| AR world display, before | 30.01 | 60.02 | 119.85 |
| AR world display, after | 30.00 | 60.02 | 120.02 |
| Native fuel-rod flight/impact, before | 29.30 | 58.33 | 119.99 |
| Native fuel-rod flight/impact, after | 29.67 | 59.99 | 119.71 |

The 60 FPS disabled controls selected no feature lights and performed no
occlusion updates. The first-person AR control retained its existing appearance.
A separate paired 60 FPS probe sweeps the real fuel-rod projectile along a
controlled path to show its surface/model spill without an impact immediately
overwhelming it; the native-flight cases above also retain collision and impacts.
These short visual captures include readback and initial shader work, and are
not sustained-performance or AMD-hardware certification.

Comparison PNGs and source-image hashes are retained locally under
`build/ar-fuel-comparisons/`; complete reports, source manifests and logs are in
`build/plasma-visual/*-ar-fuel-*`. Comparisons use matching scene, camera,
settings and simulation ticks. Only cropping, identical resizing and labels
were applied; interpolated fractions and stock particle phases can differ.


The additional CE Death Island world-display capture passed at 59.02 FPS.
An initial Marine fixture was rejected because that multiplayer map contains
no Marine tag; it was replaced by the map's stock player model, with no game
code change. Eighteen captures completed successfully overall. A hidden enabled
transition run loaded Xbox c40, MCC Mercury Falling, CE Death Island and Xbox
Blood Gulch, with three tick resets and no exceptions or forced shutdown.
This checks those map boundaries, not every custom asset or multiplayer scenario.
No new multiplayer session was required for these render-only changes; the
existing host-policy and networking harnesses passed.

The Windows portable release build was restored after each diagnostic build;
its executable contains no capture-fixture markers. Local Android/web targets
remain unavailable. [Platform CI](https://github.com/chestahh/GulchCE/actions/runs/40438187298)
passed Windows, Linux, Android and web jobs, including the Linux/web tests and
Chromium/Firefox checks. CI builds debug on this feature branch; release was
built locally. The MCC Windows adapter job passed. The separate Linux MCC
validator reached its ten-minute limit installing dependencies before tests
started. The targeted [retry](https://github.com/chestahh/GulchCE/actions/runs/40438187337)
passed both MCC jobs; no source or workflow change was needed.

Cleanup removed staged maps/caches, three temporary diagnostic executables,
fixture objects and approximately 892 MiB of redundant images/binaries. All
121 archived test-save files passed SHA-256 verification. About 147 MiB of
comparison images, representative frames, complete logs/manifests and archived
saves remain, plus the normal build. User maps, saves, previous playtest packages
and earlier milestone evidence were preserved.
