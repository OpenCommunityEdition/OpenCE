# Opus 5 safe-subset publication handoff — 2026-09-13

## Result

Five independently admissible packets were replayed onto GitHub canonical base
`73e97eca51425495acb9dbfa3349370c7962c022`. They add **12 strict exact
functions**, **5,331 meaningful code bytes**, and **5,408 padded code bytes**
with **zero strict regressions**. The accepted ledger moves from 6,960 functions
and 1,134,830 meaningful bytes to 6,972 functions and 1,140,161 meaningful
bytes.

This publication deliberately excludes the HUD packet. Its 494-byte exact
caller depends on a large fuzzy `_crosshairs_draw` rewrite whose authenticity
and raw bitmap-type constants still require separate review. No HUD credit is
claimed here.

## Admitted source packets

| Unit | Exact functions gained | Meaningful | Padded |
| --- | ---: | ---: | ---: |
| `source/ai/path` | 1 | 1,184 | 1,184 |
| `source/effects/particle_systems` | 4 | 959 | 992 |
| `source/game/players` | 1 | 1,754 | 1,760 |
| `source/interface/ui_widget` | 4 | 1,039 | 1,072 |
| `source/sound/sound_manager` | 2 | 395 | 400 |
| **Total** | **12** | **5,331** | **5,408** |

The complete function list, byte measurements, relocations, normalized hashes,
source hashes, and exclusions are recorded in
`opus5_safe_subset_admission_manifest_20260913.json`.

## Lead source-authenticity rulings

- Path, Players, and Sound use coherent natural control flow, authenticated
  names and types, and no fake compiler controls.
- Particle Systems' component-wise velocity scaling is independently supported
  by January's member-addressing pattern and HCEA's open-coded three-component
  multiplication. It is not admitted merely because it suppresses a surplus
  `_scale_vector3d` COMDAT.
- UI's two private reload helpers have authenticated static names, signatures,
  and plausible flag-owned call sites. Their one-byte return bodies are exact.
  Two comments speculating that reloading was "compiled out" were removed
  because no first-party evidence establishes that cause; the source cleanup is
  code-generation neutral.
- `_player_examine_nearby_objects` receives no credit. Its provisional closure
  depended on a rejected private-ABI edit and was reverted.
- No fuzzy function receives exact credit.

## Verification

Run from the clean integration worktree after `python configure.py`:

```text
ninja
  PASS; final progress 1,140,161 / 2,198,102 meaningful code bytes,
  6,972 / 11,060 accepted functions

python -m tools.campaign.stable_verdicts snapshot scratch/publish-opus5-safe-after.json
python -m tools.campaign.stable_verdicts diff C:\halo-worktrees\opus5-50k-house-clean-20260913\scratch\opus5-before.json scratch/publish-opus5-safe-after.json
  gained 12 functions / 5,408 padded bytes; regressions 0

python -m tools.parked_functions --output scratch/publish-opus5-safe-parks.json
  465 active; 0 stale; 0 invalid

python tools/audit_object_admission.py --output scratch/publish-opus5-safe-admission.json
  0 candidates; 0 contradicted; 7 unchanged pre-existing rejections

python tools/fake_match_scan.py --format json source/ai/path.c source/effects/particle_systems.c source/game/players.c source/interface/ui_widget.c source/sound/sound_manager.c
  0 findings

python -m pytest -q --basetemp scratch/pytest-publish-opus5-safe
  1151 passed, 5 skipped, 26 subtests passed

python tools/campaign/gate.py <each changed unit> --all --forbid-emitted-symbol _point_from_line3d
  all five gates passed; all 12 admitted rows exact

git diff --check
  PASS
```

The first unconstrained pytest invocation failed only because the sandbox could
not enumerate the default `%TEMP%\pytest-of-isabe` directory. Re-running the
identical suite with its temporary directory under `scratch/` passed fully.

No header, compiler flag, configuration, symbol map, admission policy, park, or
tool was changed. The copied target binary, Xbox SDK junction, build products,
and `scratch/` evidence remain ignored and are not part of the publication.
The 10,000-byte treemap-refresh cadence was not crossed by this 5,331-byte
subset.
