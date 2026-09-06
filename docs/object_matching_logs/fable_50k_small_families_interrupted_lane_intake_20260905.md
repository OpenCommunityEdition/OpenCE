# Interrupted Fable 50k small-families lane intake (2026-09-05)

## Scope and result

This is a read-only intake of the interrupted Fable lane and its shadow audit
tree. No donor source, shadow artifact, canonical source, target metadata, or
build configuration was changed while preparing this record.

The retained Fable branch contains a substantial set of useful source
reconstructions, but its tip is **not** the zero-regression source state
described by its own final snapshot and handoff prose. The branch must be
reconciled function-by-function; its sole commit is not safe to cherry-pick as
a unit.

The proposed fresh Opus implementation reservation is Git-diff-disjoint from
both the Fable commit and its tracked dirty work:

- `source/rasterizer/**/*.c`
- `source/devices/*.c`
- `source/objects/widgets/*.c`

That narrow fact is not a complete ownership/intake clearance. The interrupted
Fable brief explicitly reserved `source/devices/devices.c`, while the older
Opus tree retains ten `PENDING` rasterizer implementations in
`scratch/pending_files2.txt`. `source/objects/widgets` is the only one of the
three broad families that is also clear of a distinct saved source proposal.
The remaining Fable material in these families is reconnaissance rather than a
new implementation, but it still needs to be read before overlapping work.

## Main lane identity

- Worktree:
  `C:\halo-worktrees\fable-50k-small-families-20260904`
- Branch: `fable/50k-small-families-20260904`
- Tip:
  `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`
- Tip subject: `Reconstruct 82 small-family leaves across 19 objects (fable-50k batch 1)`
- Tip author/date: Jonas Volman, `2026-09-05T01:38:41-07:00`
- Sole parent and merge base with the current canonical line:
  `96086c9c4c187dd539af13bdaa9252d6e46ad8f6`
- Upstream: none configured for the Fable branch.
- Shared Git common directory:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\halo-exact\.git`

At intake time the canonical reconciliation worktree was at
`e98e3ff62852c1031f9ff74101eb0396136b42d9`. Neither that tip nor the Fable tip
is an ancestor of the other; both descend from `96086c9c4`. The Fable ledger's
statement about the branch names and `bnunu/halo` at fetch time is retained
provenance prose, not a network-verification result from this audit. The shared
repository currently has `origin` pointing to `punpckhdq/halo` and `jonas`
pointing to `bnunu/halo`.

The tip changes 27 files, with 6,714 insertions and 127 deletions:

- 19 C files;
- six headers;
- `config/symbols.json`; and
- the campaign ledger.

No Fable tip file is byte-identical to its current canonical counterpart. Six
tip paths have also changed independently in canonical history since the common
base: `config/symbols.json`, `source/ai/actions.c`,
`source/ai/actor_perception.c`, `source/ai/ai_scenario_definitions.h`,
`source/ai/encounters.c`, and `source/interface/hud_draw.c`. This does not by
itself invalidate the Fable bodies, but it rules out file-level replacement.

## Preserved campaign records

The primary handoff is still explicitly `Status: IN PROGRESS`:

- `docs/object_matching_logs/fable_50k_small_families_campaign_20260904.md`
  SHA-256
  `c76deaf7eaaa453d35e2166b0c9d06c5e86ad5bb3a5eb655977e3dcbec1d1b02`
- `scratch/WORKER_BRIEF.md` SHA-256
  `dc2d4f64eaebcbe93f644947925a9cfce803833feac147bfa75ab48da2d7d51d`
- `scratch/fable50k-before.json` SHA-256
  `3ff6acc6fc750518091474877116ed8c5fff01c2ae87b71c27372e55f10b0833`
- `scratch/fable50k-batch1c.json` SHA-256
  `f0a084485b8c894100a8bc6339aa160704c7b819420717954c3ad93948b75426`
- `scratch/verdicts-diff-batch1c.txt` SHA-256
  `f3cb9359182f98083a7de0dd2ac1dbd52f59d3c0b6e5e3efa735cd2c90451202`
- `scratch/scout-batch1c.json` SHA-256
  `d3de15cd46fc65052bdd05521b65e890ec0d011225e70e8048b69b4ef1bcd914`
- `scratch/fable50k-admission-batch1c.json` SHA-256
  `b16ff645e0093beeff2438a6bf792bfb3d0a2c68c445f9d635add423f8b4790a`
- `scratch/fable50k-parks-batch1c.json` SHA-256
  `9b5d264ac5a885eb39430340feb3b85b16f73551a4dcc2d4bf455d63bad55ece`

There are 43 per-unit Markdown reports in `scratch/reports/`. Both worker
launches ended at the account usage limit before any structured worker return,
and the campaign ledger says no independent reviewer agents ran. The reports,
source diff, objects, and snapshots therefore are evidence to replay and
review, not an independently accepted batch.

## What the retained strict diff actually claims

`scratch/verdicts-diff-batch1c.txt` records 82 strict gains, 20,096 padded
target-section bytes, and zero regressions. Grouped by object, its claims are:

| object | claimed strict functions | claimed padded bytes |
| --- | ---: | ---: |
| `bitmaps` | 18 | 4,736 |
| `error_geometry` | 10 | 3,056 |
| `weapons` | 6 | 2,112 |
| `action_obey` | 11 | 2,080 |
| `hud_draw` | 7 | 1,504 |
| `particle_systems` | 5 | 1,264 |
| `decals` | 4 | 1,120 |
| `render_cameras` | 5 | 1,040 |
| `model_animations` | 3 | 896 |
| `geometry` | 4 | 752 |
| `actor_moving` | 2 | 592 |
| `path` | 2 | 384 |
| `action_vehicle` | 2 | 256 |
| `structures` | 2 | 256 |
| `hud_weapon` | 1 | 48 |
| **total** | **82** | **20,096** |

Four C files modified in the tip have no gain in that strict list:
`actions.c`, `actor_firing_position.c`, `actor_perception.c`, and
`encounters.c`. Their edits cannot be counted as exact implementation progress
from this artifact alone.

The campaign prose separately reports +85 credited functions and +20,294
meaningful credited bytes. That aggregate is not the same measure as the saved
82-function/20,096-padded-byte strict diff, and this intake found no committed
parked-ledger update that explains the extra three functions. It must not be
restated as an independently verified strict gain.

## The tip/snapshot contradiction

The clearest state mismatch is `source/ai/path.c`:

1. The Fable tip removes the inherited 16-byte public `paths_initialize` body.
2. The same committed file retains private `closest_point_to_attractor` and
   its `path_attractor_weight` caller. The private body naturally calls
   `point_from_line3d`, creating precisely the surplus-helper boundary that the
   handoff says was removed.
3. `scratch/fable50k-batch1c.json` records `paths_initialize` as exact and the
   two attractor functions as residual. Thus that snapshot was made from a
   different source state than the committed tip.
4. The current dirty worktree re-adds `paths_initialize`, but the preserved
   `scratch/path_now.obj` was written at 01:40:43, before `source/ai/path.c` at
   01:42:48. It is not object proof for the current dirty restoration.
5. A later `scratch/fable50k-psren.json` records all three path owners as
   unwritten, further confirming that the retained reports are observations of
   multiple intermediate trees rather than one reproducible final state.

This is a source-state provenance defect, not a claim that the two genuine
`path_3d_*` exact bodies are wrong. They can be replayed independently while
excluding the attractor/helper packet and retaining the canonical initializer.

## Current dirty main tree

The main lane has six tracked modifications and 2,791 untracked entries; every
untracked path is under `scratch/`.

| tracked path | numstat | intake classification |
| --- | ---: | --- |
| `config/symbols.json` | +3/-3 | three particle-system semantic renames |
| `source/ai/path.c` | +10/-0 | re-adds the initializer removed by the tip; saved object predates edit |
| `source/effects/particle_systems.c` | +7/-7 | matching semantic renames only |
| `source/interface/hud_draw.c` | +44/-0 | two private enums and two private prototypes; no bodies |
| `source/structures/structures.c` | +78/-0 | re-adds previously exhausted planar-fog residual |
| `source/tool/error_geometry.c` | +106/-0 | two additional natural bodies, both still residual in saved object |

Important details:

- `scratch/fable50k-psren.json` SHA-256
  `9b3d7cdf74d991025d97fffa1ad6d4d92a772f47070f3a6983221b7bc360ffe4`
  preserves exact status for the two renamed small particle defaults and leaves
  the 1,664-byte update unwritten. This supports a naming cleanup, not a new
  implementation gain.
- The HUD additions declare `hud_draw_bitmap_internal` and
  `hud_draw_bitmap_with_meter` but implement neither. They are incomplete
  prerequisite notes, not a packet.
- The planar-fog body repeats the exact natural 208-byte/5-relocation topology
  already frozen and exhausted in
  `structures_obj_jonas_planar_fog_definition_index_boundary_20260829.md`.
  There is no new provenance in the dirty edit, so it is not a fresh retry.
- `scratch/w_error_geometry.obj` SHA-256
  `85659c429cad5f511a92533c0b6d2218d90794dde60e34cb9fe9cac68015e5ac`
  contains the dirty polygon-mesh and bounded-triangle attempts. Both have the
  target padded size but differing normalized hashes. The campaign did not
  update its unit report or supply a current-context admission audit for them.

None of these dirty edits should be added to the committed 82-function claim.

## Shadow audit tree

- Worktree: `C:\halo-worktrees\fable-50k-shadow`
- State: detached HEAD
- `HEAD == ORIG_HEAD == 96086c9c4c187dd539af13bdaa9252d6e46ad8f6`
- Staged changes: none
- Tracked unstaged paths:
  `source/ai/ai_scenario_definitions.h` and `source/ai/encounters.h`
- Untracked entries: 319, all under `scratch/`
  (152 `_gate_*.c`, 150 `_gate_*.obj`, and 17 other audit artifacts)

The two dirty shadow headers byte-match the Fable tip versions. Their hashes
are respectively
`cbf4e5bfe9aed171985442827247d87bb3f64e0762a6a6b37d8ff327ac3b6aef`
and
`34de75cca9107b29fb33ce8c4fca4b7f2362f003e58783d69cd1a21b11deb361`.

Retained blast evidence is mixed:

- `scratch/blast_log.txt` SHA-256
  `e86115be7dacbdccd25a39294820f3eda5e7388946e77f6135afdad56581ed5b`
  records zero losses for the AI scenario/encounters header trial.
- `scratch/blast_geo_log.txt` SHA-256
  `0a91b79342ca121dc9bebcd55a487f1bba9ae193744d8bfb1d1e58f5efb59731`
  records zero losses for the 48-consumer `geometry.h` trial.
- `scratch/blast_ps_log.txt` SHA-256
  `c8bcb3f1155ee73fe5d73c68b05928f1d492c79b6935030ebe692dda9b4ea0e9`
  records particle-system gate failures and `LOSSES/FAILS: 11`, including ten
  inherited exact owners in `particle_systems`.

Therefore the campaign ledger's blanket statement that every shared header was
blast-audited with zero losses is not authenticated by the retained shadow
artifacts. A later successful particle trial may once have existed, but this
intake found no saved successful log for it. The particle source/header packet
requires a fresh coherent current-context trial.

The retained logs also do not include a distinct saved blast for the
two-consumer `error_geometry.h` change. That header must be audited in the
current graph when its bodies are replayed.

## Reconciliation order

The table above ranks raw claimed throughput, but the recommended actionable
order accounts for source ownership and retained evidence:

1. **`tool/error_geometry` committed natural graph** — replay the ten claimed
   exact functions together with their source-required polygon residual,
   excluding the two dirty residual
   additions. The report is detailed and the packet yields a claimed 3,056
   padded bytes. Reconstruct its semantic private name and two-consumer owning
   header coherently, then run a fresh full owner/header audit; do not rely on a
   nonexistent retained error-header blast log.
2. **`ai/action_obey`** — a single-TU claimed 11-function/2,080-byte private
   caller cluster with a detailed per-function report and no committed shared
   header edit. It should be replayed against the now-evolved actor/action
   schemas and checked for every emitted inline/helper owner before admission.
3. **`bitmaps`** — the largest raw claim (18 functions/4,736 padded bytes), but
   not the first integration packet. Its report says the public
   `bitmap_mipmap_get_pixel_count` prototype belongs in `bitmaps_mipmap.h`,
   while the commit retains a C-local declaration and no
   `scratch/hdr/bitmaps.md` exists. Resolve that true owner API and the current
   protected bitmap ownership boundary before compiling the bundle.
4. **`render_cameras` or the two `path_3d_*` leaves** — smaller, separable
   current-context replays after checking natural math COMDAT ownership. The
   latter must explicitly exclude both attractor residuals and the surplus
   `point_from_line3d` owner.

Weapons is large but carries an unresolved return-ABI/header consumer note for
`weapon_place` in its own report, and Particle Systems has the retained failed
header blast. They are valuable later packets, not the safest first replay.

No donor-reported byte count in this section is outstanding admissible credit.
Each is a claim to re-establish from a clean canonical baseline with a complete
code/data/COMMON/linkage census and the current no-surplus-helper guard.

## Fresh Opus scope conflict check

Neither `HEAD^..HEAD` nor the six tracked dirty edits contains a file under:

- `source/rasterizer/**/*.c`;
- `source/devices/*.c`; or
- `source/objects/widgets/*.c`.

That is only a Git-source-diff result. The original Fable worker brief
explicitly reserved `source/devices/devices.c`, so the full Devices glob was
not independently clear even though no Devices worker launched. The three
other direct `source/devices/*.c` units with target code were already strict
complete at this intake. The older Opus source branch is
`eeedd72e5ba0fc1761519f68cad241fe606320e7` (implementation ancestor
`bb0d690a3`); its `scratch/pending_files2.txt` SHA-256 is
`897c8a12267f732617abd258dab2ebc14e3edfbfe511484fe683bde3f040de41`
and lists ten still-`PENDING` rasterizer implementations, alongside ten other
rasterizer entries already marked superseded. Those retained implementations
must be reconciled rather than treated as scratch-clear.

The remaining retained Fable material in the proposed Opus families is
reconnaissance:

- `scratch/facts/glow.md` and two `w_glow_disas_*.txt` files;
- baseline-only reports for `rasterizer_xbox_models` and
  `rasterizer_xbox_active_camouflage`;
- a fact sheet for `rasterizer_xbox_widgets`; and
- broader rasterizer fact/disassembly notes.

There is no additional non-gate C/H/include proposal in those Fable scratch
paths. `source/objects/widgets` is therefore scratch-clear. The broad
rasterizer and Devices globs are only Fable-source-diff-disjoint, not proven
free of older-lane implementation or reservation claims. In every case the
ordinary shared-owner/header coordination rule still applies.

For completeness, the initially considered `source/text/**`,
`source/memory/**`, and `source/tag_files/**` families are also absent from the
Fable commit and tracked dirty diff. Their rejection for the new Opus prompt was
throughput-based, not a Fable overlap.

## Closeout

Preserve both Fable worktrees and all scratch artifacts unchanged. Reconcile
from the combined evidence set—tip source, dirty source, per-unit reports,
snapshots, and shadow logs—rather than privileging the commit or the handoff
prose. The exact path mismatch and failed particle header blast are sufficient
to reject wholesale integration, but they do not erase the individually useful
natural bodies that can be re-reviewed and replayed in current canonical
context.
