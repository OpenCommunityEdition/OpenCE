# `damage.obj` small vitality-leaf recovery (2026-09-04)

## Result

This isolated packet advances `source/objects/damage.obj` from **19 / 34**
strict-exact functions to **22 / 34**, with no residual functions. It closes
three formerly unwritten January owners:

| January/source owner | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_object_double_charge_shield` | 93 / 96 | 3 | `ecff57a98d87a92ecb5d810f9331c6094c30477e6cb5604a725e5e7e94c8eef2` |
| `_object_permutation_shield_regions` | 126 / 128 | 4 | `c7d666ab8df0a76ce0a484f1a84edd4bb7a6d65df70e27d0e4a2f1a667a65f81` |
| `_object_deplete_shield` | 126 / 128 | 5 | `9859787bc2cf74bf2bd5c237c3d127d8483d80593ab7dd320dafbde8b0fa2608` |
| **Gain** | **345 / 352** | **12** | |

The unit's meaningful matched-code count moves from 1,282 to 1,627 of
10,242 bytes. Campaign-wide credited code moves from 884,597 / 6,073
functions to 884,942 / 6,076. Matched data remains 2,018,310 bytes; this
packet changes no data contents or data credit.

The work was based on canonical `84144bedfeea5de722e439d4e3718cf8f6928bb0`
in the isolated branch `agent/damage-small-leaves-20260904`. Neither the
canonical checkout nor any donor checkout was edited, and this packet is not
pushed by the worker.

## Authority and provenance

The pristine January split object is
`C:\halo-worktrees\fable-small-families-20260901\build\split\source\objects\damage.obj`,
SHA-256
`3fe5495d2a6ce3075c3fa6b04782cb7bbf97c3a3890f2e6f6f4f29ef8e5b08ec`.
January's sections, instructions, relocations, ABI, and object ownership are
authoritative. The lane-local split has a different raw hash after the two
semantic symbol renames described below; its code and relocation destinations
are unchanged.

The full local Fable, Opus, Claude, and reachable Git histories were searched
read-only. None contains an unintegrated implementation of these owners.
The principal semantic corroboration was:

- Stian Halo `src/halo/objects/damage.c` at
  `a613ab54a68366ad1053d1aa913ddf687a14df8e`;
- HaloCEA at `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, especially
  `object_double_charge_shield.c`, `object_deplete_shield.c`, and
  `object_permutation_shield_regions.c`;
- the existing exact damage siblings, typed object/tag accessors, January
  relocation graph, and target instruction streams; and
- the four earlier `damage.obj` ledgers plus the campaign matching,
  throughput, and acceleration documentation.

No `CLAUDE.md` or `AGENTS.md` exists at the current tree, so the latest paired
historical copies at `76ed248678d92b401b8ff73913d756050129481f` were reviewed.
Their applicable binary-first, ABI/layout-preservation, evidence, x87, and
small-commit principles are enforced here; commands for their obsolete legacy
repository layout are not applicable to this campaign tree. Aerocatia's
restored archive and open Marathon were checked as possible corroboration but
contain no direct implementation of this Halo-specific damage cluster.

## Recovered source

### `object_double_charge_shield`

The retained ordinary C computes one boolean applicability result from
`shield_vitality <= 1.f`. On success it sets the typed
`_object_shield_over_charging_bit`, raises an exactly empty shield to `0.01f`,
clears `shield_stun_ticks`, and returns the result. This is the complete
January behavior. HaloCEA's additional `force_shield_update = TRUE` belongs to
the later build and is deliberately excluded because January and the PC donor
both omit it.

The previous wave had parked a 96-byte, three-relocation near-match. This lane
reopened it with the two independent source donors and tested five bounded,
semantic spellings:

| Experiment | Result |
| --- | --- |
| direct success and failure returns | 96 padded, but one extra instruction and the wrong result lifetime |
| false-initialized result assigned in the success arm | emitted an extra `xor dl,dl` |
| block-local true constant | returned to the direct-return residual |
| branch-assigned uninitialized result plus early failure return | returned to the direct-return residual |
| retained comparison result, one success block, terminal return | strict exact |

The accepted form is the simplest complete source form that also explains
January's early `mov dl,1`; no optimizer-only carrier is retained.

### `object_permutation_shield_regions`

HaloCEA independently authenticates this semantic name and the call from
`object_deplete_shield`. It strongly maps January's address-only
`_code_00126250` to the same private helper. `config/symbols.json`, the source
definition, prototype, and top symbol inventory are renamed together, and the
owner is correctly `static`.

The helper uses `object_get`, `object_definition_get`,
`collision_model_definition_get`, and `TAG_BLOCK_GET_ELEMENT`; no raw tag or
object casts are repeated at use sites. A TU-private 0x54-byte
`struct damage_region` gives typed access to the January-proven flag and
permutation block, guarded by a compile-time size assertion. The switch-like
flag role uses the authenticated
`_object_region_missing_when_shield_is_zero_bit` enum constant and the
`TEST_FLAG` macro. The natural private definition and real caller produce
January's EAX/stack private ABI without a calling-convention directive.

### `object_deplete_shield`

The public function resolves the typed object definition and collision model,
spawns the shield-depleted effect when present, clears current shield damage,
sets `_object_shield_depleted_bit`, and calls the private region-permutation
helper once. The public declaration now lives in `objects/damage.h`; the
caller-local declaration was removed from `units.c`, which already includes
the owner header.

The damage translation unit's existing 0x4C aggregate is also renamed from
the address-derived `bss_00456e48` to the convention-compatible
`damage_globals`. HCEA authenticates its `debug_object_index` field, while the
aggregate name itself is an explicit convention-based inference because no
January private symbol survived. The config symbol and source inventory were
updated together. After regenerating the split, the pre-existing
`damage_initialize_for_new_map` remains strict exact.

## Deliberate exclusions and fuzzy boundary

- `_area_of_effect_cause_damage` was not isolated through a false external
  declaration. January proves that it calls the still-unwritten 1,316-byte
  same-TU helper at `_code_00128150`. Stian supplies an exceptionally close PC
  wrapper, and HaloCEA authenticates the helper name
  `area_of_effect_cause_damage_to_object`, but the private helper and caller
  should be reconstructed as one truthful cluster. HaloCEA's 72-result array
  and double-radius cheat are later-build differences; January requires 64
  object indices and no cheat branch.
- A bounded `render_debug_object_damage` reconstruction reached the complete
  464-byte envelope, the 134-instruction January stream, all 34 relocations,
  and matching normalized bytes before final target-symbol regeneration. It
  was removed rather than broaden this expedited three-leaf packet with six
  new subsystem includes and the external `debug_damage` declaration. No
  unverified debug code is retained. The next lane should start from the
  recorded typed shape, regenerate the target after naming `damage_globals`,
  and perform the full kept-state gates.
- No part of Stian's raw-address lift was copied. The remaining large damage
  functions, raw offsets, later-build fields, and unauthenticated address names
  are not admitted by this packet.

These are scope deferrals, not claims that the functions are semantically
unrecoverable. No new fuzzy function or parked record is needed for the three
retained owners because all three are strict exact.

## House-rule audit

Every new parameter is on its own line and every new function has an explicit
terminal `return`. The retained code uses project `real`, `boolean`, enums,
flag macros, typed object/tag accessors, and the owning header. Private linkage
and the inferred global name are semantic; there is no address-derived live
identifier in the touched owners. No assembly, volatile/register steering,
barrier, pragma, forced inline/noinline, dummy dependency, raw-byte emission,
representation pun, undefined behavior, fake branch, or nonsensical matching
construct is present. The packet introduces no `point_from_line3d` call,
symbol, or COMDAT and therefore preserves the January inline schedule.

## Verification

- Full `ninja halobetacache_build libcmt_build semantic_progress progress`:
  pass after the final source, header, and symbol changes.
- Focused `damage.obj` gate: **22 exact / 0 residual / 12 unwritten**; all 19
  inherited exact owners remain exact.
- Rename-stable whole-tree snapshots: 8,245 functions, exact count 6,121 to
  6,124; **+3 functions / +352 padded bytes / zero regressions**.
- Semantic report: 473 units scanned, 6,502 functions evaluated, 6,149
  accepted exact, zero unit errors.
- Units fixed sentinel: `_unit_preprocess_node_orientations` remains exact at
  1,920 padded bytes; the complete focused Units gate remains
  **189 exact / 0 residual / 0 unwritten**.
- `python -B -m tools.parked_functions`: 226 active / 0 stale / 0 invalid.
- `python -B tools/audit_object_admission.py`: zero candidates,
  contradictions, or revocations; the two inherited reviewed rejections are
  unchanged.
- Changed-file fake-match scan: zero review leads.
- `python -B -m pytest tools -q -p no:cacheprovider`: **261 passed**.
- Rebuilt `damage.obj` binary symbol scan: no `point_from_line3d`.
- `git diff --check`: clean apart from Git's informational line-ending
  warnings.

The pre-edit TU regression manifest intentionally cannot be compared raw after
renaming a target function symbol and the target BSS symbol: it reports target
evidence and symbol-set changes. The rename-stable section-identity sweep is
the applicable no-regression oracle here and reports exactly the three
intended gains with zero losses.

Lane-local final artifacts:

- regenerated semantic target object SHA-256:
  `E3139042F1F0D3CA250DEAEFA929277AD2A7A10245B2171C354D9289AC6812E9`;
- rebuilt candidate object SHA-256:
  `4D04FEE79CEEB1A38767A70A5345B76F1BC75ED07D9CAD52B7A4EB74DEFEAD7B`;
- final `damage.c` SHA-256:
  `4FCE5930EFD17FD327AE8DDBC0397651D7603E5702EB146C5CA18EB3F0B44E33`;
- stable snapshots:
  `audit-artifacts/damage-small-leaves-20260904/baseline.json` and
  `audit-artifacts/damage-small-leaves-20260904/final-semantic.json`.
