# `weapons.obj`: admitted safe trio and parked charged/export pair

Date: 2026-09-01

Baseline: `c59115611bad57ba03e31ae47660825608807002`

Claude source commit reviewed: `1ed2fd9efa4a322edd654d5fe9caf47ed2fe8e59`
(`claude/breakable-surfaces-20260830`).  The donor commit was not cherry-picked
as a unit; each function was reconstructed and gated independently against the
January object.

## Admitted functions

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_weapon_new` | 400 | 14 | `cdf323e6958d1ae3a767f7b2b6a4a51d44fc22926e50d17da6e3427a27331f0c` |
| `_weapon_build_weapon_interface_state` | 320 | 8 | `797487e4165a50e7bab9e68ff66ceb2d1caf551c73547ba84e10e1dc3f078f1a` |
| `_weapon_get_first_person_animation_time` | 432 | 11 | `ba16f0d911c3756014f66edf15401a0f324bbed44daddcdaf7dd4d77b17852fd` |

Hardened `tools/coff_compare.py` reports `all_equal: true` for all three.
Together they add 1,152 padded bytes, 1,125 meaningful code bytes, and three
strict-exact functions.

The implementation uses the PDB-authenticated public names and keeps the HUD
state layouts translation-unit local.  `weapons.h` owns the public prototypes;
`object_types.c` now includes that owner header instead of redeclaring the two
callbacks it consumes.  The existing `weapon_export_function_values`
declaration was moved to its owner header even though its implementation is
still parked.

The `weapon_type` and `shotgun_reload_type` values are database-authenticated.
The timing-mode names are descriptive reconstructions of the target's two
switch values.  The first-person function preserves the January code shape:
all three shotgun reload-animation records are resolved, while the handled
first-round and first-and-last-round cases both select the enter animation.
That oddity is deliberate binary fidelity, not a guessed switch-table label.

## Evidence

January PDB names:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/pdb200-jan-evidence.json/pdb_symbols.json:18043`
  (`_weapon_get_first_person_animation_time`)
- the same file at line 18083 (`_weapon_new`)
- line 18099 (`_weapon_export_function_values`)
- line 18123 (`_weapon_build_weapon_interface_state`)

HCEA name, semantic, and layout evidence (used as cross-build context, not as
a substitute for the January x86 object):

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/blam/items/weapon_new.c`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/blam/items/weapon_build_weapon_interface_state.c`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/blam/items/weapon_get_first_person_animation_time.c`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/headers/weapon_interface_state.h`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/headers/weapon_interface_magazine_state.h`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/headers/animation_graph_first_person_weapon_animations.h`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/headers/shotgun_reload_type.h`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/headers/weapon_type.h`

Pastudan independently contains readable lifts of `weapon_new` at line 1514
and `weapon_build_weapon_interface_state` at line 1769 in:

`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/pastudan-halo-full-history-20260828/src/halo/items/weapons.c`

Neither `fable/exact-finish-20260829` nor
`fable/small-families-20260901` contains any of these public function bodies.

## Rejected atomic pair

The donor's `_code_000ead20` is identified by HCEA and the authenticated zoom
ledger as the private `weapon_trigger_get_charged_fraction`.  Renaming both source and target
inventory to that semantic name preserves its exact function comparison.
The helper and its only reconstructed consumer are code-exact as an atomic
pair:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_weapon_trigger_get_charged_fraction` | 144 | 8 | `26e7b612bdcdb8bf3a44017fc2c4192291944140b01273d19d34e327816f0eae` |
| `_weapon_export_function_values` | 912 | 31 | `1f408000df04ee0db2ee5b5677e13c5a036303de69ccf4b58c388d4a28651c84` |

They are not admissible yet.  January records `__real@3d088889` (the
single-precision 1/30 constant) as an undefined external symbol, while VC7
compiling the ordinary-C helper emits a definition in candidate `.rdata`
(donor section 70).  Thus the code-only comparator is exact but the complete
translation-unit owner set is not.  No credible ordinary-C spelling found in
this audit retained the exact code while suppressing that select-any literal
owner.  The helper, export body, enum used only by that body, and temporary
target-symbol rename were therefore removed together.  Relevant semantic
sources are:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/blam/items/weapon_trigger_get_charged_fraction.c`
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/blam/items/weapon_export_function_values.c`

## Verification

- `tools/campaign/gate.py source/items/weapons --all`: 46 exact, 2 residual,
  31 unwritten (baseline 43 exact).
- `tools/campaign/verdicts.py --diff`: exactly the three gains above and zero
  regressions across 8,245 target functions.
- Full Ninja build and semantic report: 472 units, 5,146 evaluated, 5,015
  semantic exact, 5,026 accepted exact, zero unit errors.
- Aggregate progress: 651,739 / 2,198,102 meaningful code bytes and 4,983 /
  11,060 functions.
- Direct `weapons.h` blast gates preserve all prior exact counts, including
  `units.obj` 189/189, `objects.obj` 121/121, `object_types.obj` 31/31,
  `game_engine.obj` 166 exact, and `players.obj` 52 exact.
- The complete-owner comparison found no appeared, vanished, or content/owner
  changed runtime non-code section after ignoring COFF's object-local section
  ordinals.  The baseline freeze still reports expected additive code/debug
  inventory and callers changing `_weapon_get_first_person_animation_time`
  from undefined to the new same-TU definition; their runtime code and
  relocation destinations remain exact.
- `tools/fake_match_scan.py --fail-on-findings` over all changed source/header
  files: zero findings.
- `python -m pytest -q --basetemp build/pytest-weapons-packet-20260901`:
  255 passed.
- Parked-function validator: 63 active, zero stale, zero invalid.
- Object-admission audit: no `weapons.obj` contradiction or premature
  completion candidate.
- `git diff --check`: clean.
