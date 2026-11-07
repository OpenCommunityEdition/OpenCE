# OpenCE release 562 integration review

Reviewed 6-7 November 2026. This record covers the authorized merge into
GulchCE testing; it does not approve promotion to main.

## Inputs and boundaries

- Testing base: `c9014bea1af3654812a3a5c1573c23cfaaa41547`.
- OpenCE main: `fc9deb0b10eaae1d62caedb0710968b4e77e6e16`, release PR
  [562](https://github.com/OpenCommunityEdition/OpenCE/pull/562).
- Merge: `9871f523eb958c05e6ae6d3bed4ccd2a0946f53c`.
- Follow-ups: `7d18c17b6` (platform layout and integration fixtures) and
  `ab85031c8` (generated frontend controls).

This is an upstream merge plus focused fixes to existing render behavior.
Xbox and Custom Edition loading paths remain independent of MCC. MCC guards,
script extensions, Workshop discovery and package code are preserved. The
changes introduce no new MCC compatibility behavior. Network protocol/version
is unchanged: the additional fixes affect render caches and clocks, not wire
formats or shared simulation rules.

## Decisions for each incoming PR

PR numbers below refer to OpenCE. The release merge also contains its direct
follow-up commits; those are included in the resulting tree review.

| PR | Subject | Integration decision |
| --- | --- | --- |
| 441 | Browser ping ordering and progress | Take, including the queued-request delay correction `a06b4204`. |
| 522 | StageKit executable replacement | Take the ETXTBSY fix. |
| 525 | Controller remapping | Preserve GulchCE's existing fixed implementation (`f1e6ec`). |
| 526 | Controller dead zones | Take with stock `-1` defaults; regenerate CE and modern menus, preserve platform-specific row placement and footer actions. |
| 530 | First-person melee interpolation | Preserve GulchCE's stronger fix `5f23f414f`. |
| 534 | Widescreen chapter titles | Preserve GulchCE's corrected implementation `06c945c62`. |
| 536 | Scope blur edges | Take the half-pixel correction. |
| 538 | Scope zoom labels | Take Xbox glyph/bounds corrections; preserve separate CE/MCC layout and GulchCE reticle sizing. |
| 539 | Server map names and immediate rotation | Take case handling and long-rotation fixes. |
| 540 | Reversed-depth bias | Preserve GulchCE's depth/shield correction `65180b24c`. |
| 541 | Lens-flare occlusion slots | Take corrected upstream ownership/visibility serial handling (`6f1a31d6`, `cdd43c65`); preserve renderer extensions. |
| 542 | Interpolated shadow center | Take while retaining existing crash guards. |
| 545 | Camera shake per tick | Take with explicit cache resets on map initialization, disposal and checkpoint restore. |
| 546 | Contrail point ticks | Combine exact nominal tick duration with GulchCE's existing aging fix; preserve per-frame physics and texture animation. |
| 547 | Flag cloth ticks | Take with explicit reset of accumulated time and last tick on map initialization, disposal and checkpoint restore. |
| 548 | Empty-handed flashlight | Take the focused local-player change; preserve optional dynamic lighting. |
| 550 | Sound Doppler | Preserve GulchCE's corrected implementation `1bd6ebbd9`. |
| 551 | Sound declick/flush | Preserve GulchCE's corrected implementation `033440749`. |
| 552 | Lightning per tick | Preserve GulchCE's stronger lifecycle/seed fix `22b34ca16`. |
| 553 | Stale script object indices | Take while retaining MCC early casts, parameter and thread hooks. |

Our contrail aging fix already existed at `5b3693666`. Upstream's contribution
is the exact elapsed tick duration rather than a sum of render-frame times.
The combined implementation retains independent frame/state deltas, advances
state transitions separately for each elapsed tick, handles double speed, and
invalidates its clock after a checkpoint restore. It does not reconstruct
historical emission poses or provide general sub-tick trail interpolation.
See [contrail timing](contrail_timing.md).

## Combined implementation review

Reviewed subsystem ownership, declarations, initialization/disposal, saved-state
callbacks, callers, generated menus/assets, documentation and regression
fixtures. Camera matrices and flag/contrail clocks live outside the saved game
buffer. They therefore require three small registrations in the existing
`game_state.c` after-load callback table; subsystem-only resets cannot observe
a restored checkpoint. Reset implementations remain in their owning modules.
Every added game-source change carries a `port:` marker.

Existing audio, first-person, lightning, cinematic-title, render interpolation,
MCC and Workshop implementation files remain unchanged from the testing base.
Shared renderer changes retain GulchCE depth, shield and lighting integration.
No unrelated refactoring was justified. Fixture adaptations account for retained
MCC interfaces, visibility ownership fields and generated controller rows;
they do not disable those production checks.

## Automated validation

Code at `ab85031c8` passed the complete
[Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42316635322):
Windows, Linux, Android and web jobs all succeeded. These branch jobs build
their configured debug variants; release/profile jobs gated on main were not
run. The local Windows build additionally used portable release, PGO off and
LTO off, LLVM 23.1.4.

- Linux's combined engine/harness/StageKit regression command: **2,429 passed,
  27 skipped**, plus seven passing subtests. The other Linux regression
  commands passed, including controller/HUD and co-op checkpoint tests.
- Web: page/importer/service-worker and build checks, Chromium/Firefox smoke
  and room tests, and native WebRTC interoperability passed. Retail-map
  browser internet gameplay was not exercised.
- Android: guest/native/app build and configured importer/touch checks passed.
- [MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42316635244):
  both jobs passed. Reader/validator: 1,407 passed/14 skipped and 36 passed;
  Windows adapter commands: 40 passed/one skipped, 1,520 passed/16 skipped,
  and 38 passed.
- Local focused effects/render/cache/web checks: **272 passed, five skipped**.
- Local audio/Doppler/cancellation/depth checks: **60 passed, six skipped**.
- Local frontend generation/navigation/presentation checks: **38 passed**.
- Lifecycle and contrail harnesses extract production functions and the real
  after-load callback table. Deliberately restoring frame-based aging or
  removing lifecycle hooks causes the relevant tests to fail. Their graphics,
  attachment and physics stubs do not prove visual or map compatibility.
- `git diff --check` and changed-document local-link checks passed.

An exploratory broad local Windows run was not green (1,642 passed, 730 failed,
13 skipped), largely because that suite assumes POSIX headers/linker flags,
CRT behavior and newline conventions. Actual integration-fixture failures were
fixed and rerun. The final Linux CI suite above is the supported full-suite
result; the exploratory run is not counted as a pass. Local Android/web ninja
targets are unavailable in the Windows-generated graph; CI supplies full builds.

## Real-engine Windows qualification

Runs used hidden windows, separate writable data/save roots, at most two
instances, original retail/CE map copies, and the unmodified `net_compare`
judge/tolerances. Original maps, Workshop packages and user saves were untouched.

- Xbox Blood Gulch, two peers: **47 matched seconds**, no reported divergence
  or judge problems, normal exits; maximum position difference 0.143.
- Local Custom Edition Death Island, two peers: **51 matched seconds**, no
  reported divergence or judge problems, normal exits; maximum position
  difference 0.0924. These are tolerance-based checks, not bit-exact equality.
- One process: Xbox b40 checkpoint save/revert, attempted Workshop Mercury
  load, CE Death Island, then Xbox Blood Gulch. The checkpoint was restored,
  CE loaded, and subsequent Xbox simulation ran with normal exit and no
  exception. Mercury's resource selection failed, so this is **not** a
  successful three-family transition test.
- Campaign co-op without a valid checkpoint cue reported aim/alive divergences.
  An exact source snapshot of the pre-merge testing commit reproduced the same
  55-degree opening aim discrepancy, later alive discrepancy and maximum
  position difference (1.2973). This establishes those observed symptoms also
  occur before the merge; it does not establish that campaign co-op is correct.

- A corrected two-peer b40 run executed `game_save` at tick 901 and
  `game_revert` at tick 1201; the host confirmed the saved game was taken.
  Both processes exited normally after continued simulation. The unchanged
  judge still reported the opening cinematic aim difference (56 degrees),
  so this run is not a clean campaign-network pass.
- The pre-merge testing binary also refused Workshop Mercury with the same
  missing matching `bitmaps.map` diagnostic. A second current-build attempt,
  Workshop `3675162225/ring_mp`, failed resource selection too. Both exited
  normally. Workshop discovery succeeded; MCC gameplay did not start.
  Inspection shows Mercury's map under the item's `maps` folder and resource
  files under `halo1/maps`, while `steam_workshop_package_folder` selects the
  map's containing folder. This is evidence for a path-selection issue, not
  proof that those resources are otherwise compatible. No package files were
  moved and no unrelated MCC compatibility changes were made.

## Promotion limits

Keep main unchanged until the remaining runtime gaps are resolved or explicitly
accepted with evidence. Green asset-free tests and builds do not establish full
MCC or campaign compatibility. No visual A/B captures were added in this merge;
the prior contrail captures documented in `contrail_timing.md` cover the earlier
aging fix, not this exact integrated binary.

Local logs, result JSON, runner scripts and losslessly archived test saves are
retained in the integration worktree's ignored `build/` folder. Copied test maps,
generated map caches and disposable baseline build outputs are removed after
qualification. The ready Windows build remains available for follow-up testing.

Cleanup retained approximately 59 MiB for the Windows executable, symbols and
SDL runtime, and 5.1 MiB of runtime evidence including verified save archives.
The temporary baseline source/build, copied maps, caches, build objects and
qualification virtual environment were removed. Logs and package-version records
remain available; no test game processes remain running.
