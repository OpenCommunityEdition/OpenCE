# Game Sound PAS: current-context reconciliation (2026-09-04)

## Scope and legitimate reopening

Starting publication is `18238d9065014b67fa2a1d745140f203be6e44dd`.
This selectively replays the preserved local packet
`9d60117aa0dbeb8a3c75f929d892cf29d8170467`, not the whole dirty Opus tree.
The original donor remains read-only at
`C:\halo-worktrees\opus-small-families-30k-20260902`, tip `eeedd72e5`.
The new Fable 50k lane is excluded from this reconciliation.

The old packet correctly withheld admission because its genuine owning
`game_sound_update(real dt)` prototype changed one 1,920-byte Units function
from exact to residual. That evidence and source remain preserved in the
local packet and its `game_sound_obj_pas_caller_cluster_blocked_20260904.md`.

Since that checkpoint, genuine HUD owner APIs and forward declarations have
landed, among other source reconstruction. A fresh probe with the *same*
two-line Game Sound prototype in its *same* natural place after
`game_sound_restore` now preserves all 189 Units functions. This meets the
old evidence-based reopening condition. It is not proof that a particular
HUD declaration alone caused the improvement: no declaration bisection,
permutation, dummy type, compiler-flag search, or Units implementation change
was used to obtain this result.

## Retained production source

- `game_sound.c`: the actual update caller and three private PAS helpers,
  using existing complete sound, object, location and BSP layouts.
- `game_sound.h`: the correct owner prototype. January `game_frame` pushes
  the frame delta even though the callee does not read it.
- `game.c`: remove the obsolete caller-local declaration; the owner header
  was already included. The actual caller is unchanged.
- `scenario.h/c`: type the real sound-environment output as a pointer to a
  const definition and remove the two pointer-through-long assignments.
  No environment interpolation logic changes.
- `sound_manager.h`: correctly owned pointer-taking environment API,
  six-argument looping refresh API, and start/loop/stop enum constants.
- `symbols.json`: map the three authentic semantic private names at January
  file offsets 1798800, 1799728 and 1799968, retaining static linkage.

The source uses typed tag/data accessors, project types, named flags and
bit-vector macros, one parameter per line, explicit returns and normal
caller-driven retention. `combined_pas` is real external COMMON64 storage:
the January contribution is module 847 / file offset 5165760, represented by
the synthetic linker-common split owner. It is not invented Structure BSP
storage. Four local players are used, as in January, not HCEA's later two.

January COFF is the authority. The independent HCEA Game Sound/Structures
reconstructions corroborate the semantics without donating later-build ABI
or timer changes. The final write after `game_looping_sound_delete` is present
in January itself and refers to fixed data-pool storage; it is not justified
merely because an earlier donor happened to match. No manual inlining,
optimizer steering, new assembly, fake references or shadow layouts are added.

## Verification and accounting

| Function | Target meaningful / padded | Candidate padded | Relocations | Result |
| --- | ---: | ---: | ---: | --- |
| `location_potentially_audible` | 102 / 112 | 112 | 6 / 6 | strict exact |
| `compute_combined_pas` | 236 / 240 | 240 | 10 / 10 | 97.5% fuzzy |
| `update_potentially_audible_looping_sound` | 619 / 624 | 640 | 25 / 25 | 83.65909% fuzzy |
| `game_sound_update` | 453 / 464 | 464 | 31 / 31 | 97.91391% fuzzy |

Game Sound advances from 26 exact / 0 residual / 5 unwritten to
**27 exact / 3 residual / 1 unwritten**. `compute_sound_obstruction` is still
unwritten. The three honest residuals are parked as unclassified, not claimed
as compiler ties or exact bytes. The old donor's 352-padded-byte PAS claim is
not imported into the accounting.

The full production build passes. The rename-stable comparison against
`scratch/hud-draw-two-leaves-after-20260904.json` preserves every inherited
exact owner and adds only target section 21 in Game Sound: **6,187 / 8,245
stable strict owners, one gain / zero regressions**. The proper shared-header
rebuild includes all dependent translation units, not just the focused sound
gate; 91 C files directly include one of the three edited owner headers.
The separate Units sentinel remains **189 / 189**.

Canonical meaningful totals advance from 898,585 to **898,687 code bytes**,
6,140 to **6,141 credited functions**, with **2,022,998 data bytes** and
**391 / 833 Matching objects** unchanged. The 1,308 meaningful bytes covered
by the three reconstructed residuals receive zero exact credit.

Complete Game Sound code census contains precisely the 30 implemented target
owners, including proper storage-class-3 private helpers, and no surplus code
owner or `_point_from_line3d` COMDAT. Its COMMON symbols are the inherited
four-byte globals plus the authentic 64-byte PAS buffer. No extra Game Sound
data credit or whole-object approval is claimed. The separate inherited Units
48-byte helper veto remains in force; this packet neither introduces it nor
waives it.

All **255 parks validate**, with zero stale/invalid entries. Admission reports
zero candidates, zero contradictions, four otherwise-complete rejections and
zero revocations. The six-file fake-match scan reports zero leads. **288 tests
and 26 subtests pass**; the optional pytest-cache permission warning is
unchanged. `git diff --check` passes and both protected bitmap hashes are
unchanged. Unrelated EOL/index-only dirty files are not staged.

## Reproduction and artifacts

```text
python tools/campaign/gate.py source/sound/game_sound --all --forbid-emitted-symbol _point_from_line3d --out scratch/pas-current-final-game-sound-20260904.obj
ninja
python -m tools.campaign.stable_verdicts snapshot scratch/pas-current-replay-after-20260904.json
python -m tools.campaign.stable_verdicts diff scratch/hud-draw-two-leaves-after-20260904.json scratch/pas-current-replay-after-20260904.json
python tools/campaign/gate.py source/units/units --out scratch/pas-current-final-units-20260904.obj
python -m tools.parked_functions --output scratch/pas-current-final-parks-20260904.json
python tools/audit_object_admission.py --output scratch/pas-current-replay-admission-20260904.json
python tools/fake_match_scan.py source/sound/game_sound.c source/sound/game_sound.h source/sound/sound_manager.h source/scenario/scenario.c source/scenario/scenario.h source/game/game.c
python -m pytest -q tools
python configure.py progress
git diff --check
```

Game Sound physical source SHA-256:
`852c5ce7d635ed9b99b2d516de40e5a9ff7df43d1d6de67776c2588cb55f9c2f`.
Production object SHA-256:
`fdef7f46c1a7d8b2913c9d44e1452e8e72b47c62bdc3a1b33d9e440fb6485bb7`.
New strict helper normalized SHA-256:
`2a9fb7234670d74e0373cb40f8370dbbcf7f369bc3310a24dd552f5adafe0885`.
Ordered relocations, not physical hashes or fuzzy percentages alone, determine
strict comparison. Scratch/proprietary binaries are not committed.
