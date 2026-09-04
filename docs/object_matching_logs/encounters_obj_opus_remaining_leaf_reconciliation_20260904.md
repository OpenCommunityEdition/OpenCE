# `encounters.obj` Opus remaining-leaf reconciliation (2026-09-04)

## Scope and accounting boundary

This packet was reconstructed and validated in the isolated worktree
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\worktrees\encounters-opus-reconcile-20260904`
from canonical commit `1c834b12f250db60ee841c151e201ffc2d706c49`.
The interrupted donor at
`C:\halo-worktrees\opus-small-families-30k-20260902` remained read-only.

The donor report advertises 448 padded bytes in
`_encounters_create_for_new_map` and `_encounters_update`. Those exact leaves
were already admitted canonically by commit `cd124bb10` and are documented in
`encounters_obj_opus_two_leaf_reconciliation_20260903.md`. They are therefore
not imported or counted again here. The donor's provisional large bodies and
unmeasured source-shape experiments were also excluded.

The actual current-base gain is two different, previously unwritten leaves:

| function | meaningful bytes | padded bytes | relocations | result |
| --- | ---: | ---: | ---: | --- |
| `_encounter_pursuit_position_already_examined` | 168 | 176 | 7 | strict exact |
| `_encounter_get_actor_starting_location` | 731 | 736 | 24 | strict exact |
| **packet delta** | **899** | **912** | **31** | **+2 functions** |

There is no owned-data change. The focused object moves from 41 exact / 4
residual / 16 unwritten to **43 exact / 4 residual / 14 unwritten**.

## Reconstruction and ABI evidence

`encounter_pursuit_position_already_examined` now performs the complete typed
pursuit lookup. A found record contributes its examiner count and last-examined
time; a position is already examined when the six-entry actor ring is saturated
or the requested actor is present. Optional output pointers receive the
measured count and time on both hit and miss paths. The `>= 7` saturation
threshold is independently present in the HaloCEA Xbox reconstruction and
agrees with January's `cmp cx, 7` branch. The final output is correctly typed
as `long *`, rather than the unrelated `boolean` placeholder that previously
occupied the public declaration.

`encounter_get_actor_starting_location` uses the semantic encounter, squad,
scenario-tag-block, random-seed, and bit-vector interfaces. It first consumes a
random required location, then selects from the unused bank, refilling that bank
when it is exhausted. January's two-long local bit vector establishes the
64-location bound used by the named local enum constant. The function's
`spawning` argument is genuinely dead in both January x86 and the independent
HaloCEA PowerPC reconstruction; retaining that authenticated quirk is not an
optimizer-steering device. Likewise, the call-local assigned-location vector is
zeroed exactly as January emits it even though this build does not subsequently
mark entries in that vector.

The second parameter is a full-width `long squad_index`. January reads the
complete stack dword in the callee, and only the full-width declaration emits
the exact 736-byte function. HaloCEA independently identifies the corresponding
PowerPC argument as an `int` squad index. This direct callee and ABI evidence
supersedes the declaration-only conclusion in
`ai_script_obj_fable_snapshot_reaudit_20260902.md`, which had called the old
`short` owner declaration authoritative before the body was available.

Both corrected prototypes live in `source/ai/encounters.h`, their owning
subsystem header. Its ten direct C includers were all rebuilt by the full Ninja
build, and the whole-tree stable comparison reports no lost exact function.

## Strict comparison evidence

The hardened COFF comparison reports equal padded size, relocation topology,
and normalized bytes for both new leaves:

| function | January/candidate normalized SHA-256 |
| --- | --- |
| `_encounter_pursuit_position_already_examined` | `0b83a0fe5034a286dbf0cea928a9810de2547cbdc9ba14d472abee5eebfcd5b2` |
| `_encounter_get_actor_starting_location` | `da94b075a4ce9127ce4f724facfe81f6d935b1e36ae18c62c5ec9043f2d809ec` |

The generated January split object is SHA-256
`7EDC1863EB2C08F7F2AD612E3CFA7B246C0EB9A3B5907FD08C2C17ECD9A056A3`;
the final rebuilt `encounters.obj` is
`7CE68F96555A8B266754025E2266CB7BD1132F9A32D8C670740F5FF528FAF26C`.
Object-level inequality is expected because four emitted functions remain
honest residuals and fourteen target functions remain unwritten.

## Honest residual effects

The corrected full-width squad ABI and the natural addition of the two real
functions improve two already-emitted callers without claiming either exact:

| function | before | after | disposition |
| --- | --- | --- | --- |
| `_encounter_place_actor` | 384 padded, 76.09% | 368 padded, 88.07% | target-size residual, all 14 relocations retained |
| `_encounter_spawn_actor` | 288 padded, 94.61% | 288 padded, 95.51% | target-size residual, all 16 relocations retained |

For `encounter_place_actor`, the old narrow declaration forced an unnecessary
argument conversion and the corrected ABI recovers January's initial call
schedule. A donor experiment that cached `global_scenario_get()` in a local
`scenario` pointer was retested on the current base; it worsened the function
back to 384 padded bytes and was rejected. For `encounter_spawn_actor`, the
remaining difference is the compiler's early callee-save scheduling around a
fast failure branch. Neither residual is parked, credited as exact, or forced
with a register hint, barrier, assembly block, or artificial source shape.

## House-rule and source-credibility audit

The retained source uses semantic names, cseries/Halo types and constants,
typed encounter/tag accessors, bit-vector macros, one parameter per line, and
an explicit return from each function. No address-derived private name,
`bss_<address>` global, raw field offset, cast after raw `object_get`, direct raw
`tag_get`, forced inline, volatile/register forcing, compiler barrier, pragma,
assembly, fake dependency, representation pun, undefined behavior, or
nonsensical branch was introduced. There is no `point_from_line3d` symbol or
COMDAT in the rebuilt object, so the January inline schedule is preserved.

HaloCEA commit `c89106c4964f2df3a98ef7b4035d6750372797cd` supplies independent
behavior and ABI corroboration through
`src/encounter_get_actor_starting_location.c` and
`src/encounter_pursuit_position_already_examined.c`. January COFF remains the
strict byte, relocation, and ABI authority. No useful Marathon analogue was
found for these encounter-specific routines.

## Verification

- Full `ninja`: passed after the final source and both header corrections;
  semantic unit errors are zero.
- Focused Encounters gate: 43 exact / 4 residual / 14 unwritten.
- Fixed Units sentinel: 189 exact / 0 residual / 0 unwritten, including the
  1,920-byte `_unit_preprocess_node_orientations` function.
- Rename-stable whole-tree snapshots: 8,245 functions, exact count 6,067 to
  6,069; **+2 functions / +912 padded bytes / zero regressions**.
- Semantic progress: 6,021 to 6,023 matched functions and 871,741 to 872,640
  meaningful code bytes: **+2 functions / +899 meaningful bytes**. Matched data
  remains 2,015,906 bytes and linked objects remain 391/833.
- Semantic-match audit: 473 units scanned, 6,437 functions evaluated, 6,096
  accepted exact, zero unit errors.
- `python -B -m tools.parked_functions`: 212 active / 0 stale / 0 invalid.
- `python -B -m tools.audit_object_admission`: 0 candidates / 0 contradicted /
  0 revoked; two inherited reviewed rejections are unchanged.
- `tools/fake_match_scan.py --fail-on-findings` on the changed source and
  header: zero review leads.
- Tooling suite: **261 passed** with a lane-local writable pytest base.
- Protected bitmap hashes are unchanged:
  `bitmap_utilities.c` =
  `DCC516B95C8C51ED52BC245170244BE2FDC01135138D82CF1A774A0BFD09FE64`;
  `bitmaps.h` =
  `7011541E1AB8F5E0F732D7FCCC379D381F65DA6A27C2C06661AA4F3037598E8D`.
  Their Git diff is empty.
- `git diff --check`: clean apart from Git's informational LF-to-CRLF warning.

Stable snapshots:

- `scratch/before-encounters-opus-reconcile-20260904.json`
- `scratch/after-encounters-opus-reconcile-20260904.json`

