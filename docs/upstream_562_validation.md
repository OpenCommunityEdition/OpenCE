# OpenCE release 562 integration review

Reviewed 6-7 November 2026. This record covers the authorized merge into
GulchCE testing. The subsequent user-authorized promotion is recorded below.

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

The MCC resource-loading failure recorded above is resolved by the follow-up
below. At the end of that review, main was left unchanged while campaign co-op
qualification remained open. The later promotion does not close that limitation.
Green asset-free tests and builds do not establish full MCC or campaign compatibility. No visual A/B captures were added in this merge;
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

## MCC resource-discovery follow-up

The user subsequently authorized fixing resource discovery generally, including
both authored package resources and installed shared MCC resources. Code commit
`2959e99b22f1079bf3bd9c63c11578d16928fa62` implements that rule without map-name,
checksum or Workshop-ID exceptions. Read [resource installation](mcc_maps.md#subscribed-steam-workshop-items)
and the [loading walkthrough](reading/maps_and_resources.md) for precedence,
ownership and the small MCC-only virtual-file-service hook.

Review covered the entire resolver, its host-file caller, package and shared
precedence, independent types, bounded paths, disabled discovery, lifecycle,
resource validation and content-based network identity. No shared simulation
or wire format changed; resource contents still decide peer admission. No new
cache or open-stream owner was introduced. The existing resource reader rejects
invalid authored files and ambiguous shared matches rather than replacing them.
No unrelated refactoring was needed.

Seven synthetic cases failed before implementation. The final local Windows
suite covering Steam discovery, MCC resources/identity/maps/protocol, Android
imports and web build checks passed **259 tests**, with **10 platform/tool
skips**. Tests cover unrelated package identities, mixed case, per-type lookup,
local overrides, invalid/unmatched/ambiguous resources, other Steam libraries,
path bounds, forbidden paths, disabled discovery and unchanged Xbox/CE roots.
Windows portable release rebuilt successfully with PGO/LTO off.

Code at `2959e99b2` passed the complete
[Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42319635736):
Windows, Linux, Android and web, including Chromium/Firefox and native WebRTC
checks. Both jobs in the
[MCC workflow](https://github.com/chestahh/GulchCE/actions/runs/42319635900)
also passed: reader/validator commands reported 1,425 passed/14 skipped and
36 passed; Windows adapters reported 40 passed/one skipped, 1,538 passed/16
skipped and 38 passed. Platform debug/release scope is as documented above.

Real-engine checks used the same hidden, isolated, at-most-two-instance rules:

- Mercury Falling loaded its own bitmap and sound files directly from the
  Workshop item's `halo1/maps`; player simulation advanced through tick 570.
- Ring loaded the shared bitmap file directly from the installed MCC game;
  player simulation advanced through tick 720.
- Two-peer Ring: **45 matched seconds**, no divergences or judge problems,
  maximum position difference 0.2002, normal exits.
- Two-peer Bigass: **41 matched seconds**, no divergences or judge problems,
  maximum position difference 0.0736, normal exits. Both peers selected the
  package's authored bitmap and sound resources in place.
- One process completed Xbox b40 checkpoint save/revert, refusal of a deliberately
  invalid MCC package bitmap, Mercury with authored resources, Ring with shared
  resources, CE Death Island and a return to Xbox Blood Gulch. Every post-load
  simulation marker executed; the saved state restored, the invalid resource
  was refused, all intended maps loaded, no exception was reported and exit was
  normal. This closes the previously failed resource/transition check.

The invalid package was a disposable fixture, not an edit to installed content.
No Workshop resources were copied into shared folders to obtain these passes.
The executable used for these runtime checks has SHA-256
`6cb70d41816f35224b143007483c1234ca232eeecc01e33fb2c0eed2ab29f66f`.
Logs/results are under `build/runtime-562-final`, including
`workshop-mercury-fixed`, `workshop-ring-fixed`, `mcc-ring-network`,
`mcc-bigass-network` and `lifecycle-resources-fixed`. Save archives are verified
against SHA-256 manifests before removing loose inactive copies. Copied test
maps and generated map caches were removed; originals remain untouched.

These checks establish the repaired discovery paths and representative gameplay,
not complete MCC campaign compatibility. Shared sound discovery/selection is
covered synthetically; the installed shared-resource runtime example required
external bitmaps only. The pre-existing b40 co-op comparison issue remains a
separate qualification limit. Main was not promoted at that point.

## Subsequent promotion to main

On 7 November 2026, the user explicitly requested merging current testing into
main and monitoring CI. The initial merge inputs were testing
`9bf9554aa86dbeba9fa49f5d8ff1b99f2ba82c1b` and main
`c5ae133ff69701ca9d48d32808969c13afe00d9f`. That testing revision's
[platform builds and regression checks](https://github.com/chestahh/GulchCE/actions/runs/42324288419)
and [MCC validation](https://github.com/chestahh/GulchCE/actions/runs/42324288332)
passed before the conflict-free merge.

Before release publication, the user clarified that the CPU mask-conversion and
collision-continuation optimization is still being developed and must be left
out. The initial Build/release run was cancelled. A focused revert on main
removes all changes from `9bf9554aa`; testing and its ongoing work are untouched.
The corrected engine, platform and build configuration exactly match
the previously reviewed testing revision
`045cb99bce3fba18a3ddb61f88e327ba4544d09e`. Main's existing README and agent/coding
guidance remain intact, and the Workshop diagnostic branch is excluded.

The release-note collector additionally suppresses the withdrawn optimization's
original annotation only in histories containing its withdrawal, as documented
in [the changelog guide](changelog.md). A real Git-history regression reproduces
the stale note before correction and checks retained testing previews and later
restoration notes. This bookkeeping change does not alter game code.

The optimization commit remains in Git ancestry because main was already pushed;
when that work is ready, its full intended change must be deliberately restored
and validated. Merging later descendants alone will not restore reverted lines.
Fresh CI on corrected main builds debug and release configurations before
publication. The b40 co-op comparison discrepancy remains an explicitly recorded
pre-existing qualification limit, not a resolved defect or a clean network pass.

## Validated lighting follow-up

Later on 7 November, eight controlled combat runs supported retaining only the
CPU mask conversion. Testing `e96be13db711ad305cd812a8211d7894d13135ae` restored
the previous recursive collision traversal and passed its
[Build workflow](https://github.com/chestahh/GulchCE/actions/runs/42328462980)
and [MCC validation](https://github.com/chestahh/GulchCE/actions/runs/42328462944).
The user then authorised promotion of that validated work. This later promotion
deliberately restores the mask conversion, its tests and documentation; it does
not restore the discarded shared collision rewrite or alter the preceding MCC
resource work. The original withdrawn release note stays suppressed, while the
follow-up supplies its own reviewed annotation. See the
[controlled comparison](dynamic_emitter_expansion_acceptance.md#controlled-comparison-and-retained-change-7-november-2026)
for all eight results and the remaining sustained-FPS limit.
