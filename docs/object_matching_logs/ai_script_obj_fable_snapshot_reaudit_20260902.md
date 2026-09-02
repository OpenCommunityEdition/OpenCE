# `ai_script.obj` Fable snapshot re-audit (2026-09-02)

## Scope and pinned inputs

- Canonical baseline: `251fc455f51648204d0c72547957798898b43c5d`.
- Preserved Fable snapshot: `1365c04b19ef51667162d0db03ac8512ad8d1ffb`.
- Isolated raw-snapshot commit: `8b15cc1c2b9c71b301fffc976a1fb9d9a97068f6`.
- January target: `build/split/source/ai/ai_script.obj`, SHA-256
  `1A04C31FE9B02C1BDEA6D15B2210D4F75493F51EA610CCE5742C053BAE3ECA2E`.
- Measurement authority: `tools/campaign/gate.py`, an isolated `csplit` of the
  renamed symbol map, and the hardened COFF comparator. Function identity is
  the configured January address, not a report display name.

The isolated worktree was not configured with Ninja and canonical was not
modified or pushed.

## Snapshot verdict

The raw Fable snapshot is provenance worth preserving, but it is not an
admissible implementation by itself. It adds 203 lines of includes, local
type guesses, and cross-module declarations to `source/ai/ai_script.c`; it
adds no function body and no owned datum. Its gate remains exactly the pinned
baseline:

| state | exact | residual | unwritten | listed |
| --- | ---: | ---: | ---: | ---: |
| pinned canonical | 38 | 1 | 77 | 116 |
| raw Fable snapshot | 38 | 1 | 77 | 116 |

Two raw declarations are demonstrably ABI-wrong: the real owner declaration
of `biped_approximate_surface_index` returns `short`, not `long`, and the
`squad_index` parameter of `encounter_get_actor_starting_location` is
`short`, not `long`. The reconciliation therefore removes the snapshot's
copied declaration block and retains only evidence-backed local definitions.

## Reconciled result

The reconciliation restores natural C bodies previously proved exact by the
same January compiler/target lineage. It preserves every inherited exact
address and closes the sole residual by correcting the platoon-array bound in
the owning inline helper.

| measurement | baseline | reconciled | delta |
| --- | ---: | ---: | ---: |
| strict exact functions | 38 | 62 | **+24** |
| residual functions | 1 | 0 | **-1** |
| unwritten functions | 77 | 54 | **-23** |
| exact padded code bytes | 3,536 | 7,488 | **+3,952** |
| exact meaningful code bytes | 3,261 | 7,030 | **+3,769** |
| relocations in exact code | 175 | 404 | **+229** |

The 24 newly exact January owners are:

| address | target owner | padded | meaningful | relocs |
| ---: | --- | ---: | ---: | ---: |
| `0x42690` | `_encounter_get_platoon` | 144 | 130 | 9 |
| `0x42890` | `_ai_index_to_string` | 240 | 237 | 13 |
| `0x42980` | `_ai_index_platoon_iterator_new` | 288 | 275 | 9 |
| `0x42AA0` | `_ai_index_platoon_iterator_next` | 112 | 108 | 7 |
| `0x42B10` | `_ai_index_squad_iterator_new` | 256 | 255 | 8 |
| `0x42C10` | `_ai_index_squad_iterator_next` | 224 | 215 | 10 |
| `0x42E30` | `_object_list_from_ai_reference` | 160 | 154 | 7 |
| `0x43890` | `_ai_scripting_timer_start` | 144 | 137 | 9 |
| `0x43920` | `_ai_scripting_timer_expire` | 160 | 146 | 10 |
| `0x43DC0` | `_ai_scripting_attack` | 144 | 136 | 9 |
| `0x43E50` | `_ai_scripting_defend` | 144 | 136 | 9 |
| `0x43EE0` | `_ai_scripting_maneuver` | 144 | 137 | 9 |
| `0x43F70` | `_ai_scripting_maneuver_enable` | 160 | 157 | 11 |
| `0x44FF0` | `_ai_scripting_braindead` | 160 | 160 | 12 |
| `0x456A0` | `_ai_scripting_allow_charge` | 160 | 156 | 11 |
| `0x45EC0` | `_ai_scripting_force_active` | 176 | 161 | 12 |
| `0x462E0` | `_ai_scripting_playfight` | 144 | 132 | 10 |
| `0x468E0` | `_ai_scripting_automatic_migration_target` | 160 | 153 | 11 |
| `0x46980` | `_ai_scripting_follow_target_disable` | 128 | 115 | 8 |
| `0x46A00` | `_ai_scripting_follow_target_players` | 128 | 115 | 8 |
| `0x46A80` | `_ai_scripting_follow_target_unit` | 144 | 137 | 8 |
| `0x46BC0` | `_ai_scripting_follow_distance` | 128 | 121 | 8 |
| `0x46E40` | `_ai_scripting_berserk` | 144 | 143 | 10 |
| `0x46F40` | `_ai_scripting_allow_dormant` | 160 | 153 | 11 |

Address-normalized exact-set comparison reports `lost_exact: []`. A fresh
`csplit` target made from the semantic symbol map compares ordinarily as
62 exact / 0 residual / 54 unwritten against the normal rebuilt object. The
semantic split target SHA-256 is
`418DEE372C43EC72C398032311F1DD94A3ABCB90806860249B3B59BFC29C14F1`;
the normal rebuilt object SHA-256 is
`0110191F55B4733B04C70939FEA4B2DF79A7C9B36EC22341BA804961ABA86428`;
and the pinned-baseline scratch object SHA-256 is
`0D82C8B425A512E5D4664ADC4885B0C33B972EA7FCB684376AF1321B7A372DAF`.

## Source and ownership reconciliation

- `source/ai/ai_script.c` gains only strict-exact bodies and the minimum
  semantic enum/type support they require. Typed scenario tag-block, actor,
  squad, platoon, encounter, unit, and object-list accessors are used.
- `source/ai/ai_script.h` owns the iterator layout and new public prototypes.
  The squad iterator field names follow HaloCEA semantic evidence:
  `required_platoon_index` and `next_squad_index` distinguish the filter from
  the cursor.
- `source/ai/encounters.h` changes the second `encounter_get_platoon` bound
  from `MAXIMUM_SQUADS_PER_MAP` (`0x400`) to the semantically and
  disassembly-correct `MAXIMUM_PLATOONS_PER_MAP` (`0x100`). This closes the
  inherited residual without an artificial code shape.
- Raw copied prototypes, guessed `actor_starting_location` field meanings,
  and unused broad includes are removed. Existing owner declarations in
  `actors.h`, `encounters.h`, and related subsystem headers remain authority.
- `config/symbols.json` replaces the four implemented private address
  placeholders with the exact HaloCEA behavior names at the same January
  addresses: `ai_scripting_kill_internal` (`0x432B0`),
  `ai_scripting_count_internal` (`0x439C0`),
  `ai_scripting_vehicle_candidate_qsort` (`0x44EA0`), and
  `ai_scripting_go_to_vehicle_internal` (`0x47160`). The definitions,
  declarations, callers, and split owners use those names consistently.
- No inline assembly, volatile cast, raw-byte emission, optimization pragma,
  address call, artificial anchor, or new `code_<address>`/`bss_<address>`
  identifier was introduced. No implemented private function in this unit
  retains an address-placeholder name.
- Signatures use one parameter per line, no-argument declarations put `void`
  on its own line, `real` and cseries constants/macros are used, and new void
  bodies end with explicit `return;`.

Historical exact provenance includes `d0e3e01d7`, `c44035b35`, `dad10bb79`,
`0650d4bf1`, `fe74ef2c4`, `eb5585436`, `10d2cf18d`, `7e5634640`,
`986689e25`, `7d7ee8650`, and `0dfd63498`. HaloCEA, PDB, and Stian material
were used only for naming/topology evidence; January COFF remains byte and ABI
authority. No direct Marathon analogue was found.

## Header blast audit

`ai_script.h` has three direct includers and `encounters.h` has ten. Every
unique direct includer was gated after the final header state:

| translation unit | exact | residual | unwritten | delta from baseline |
| --- | ---: | ---: | ---: | --- |
| `source/ai/actions` | 58 | 5 | 0 | unchanged |
| `source/ai/action_search` | 9 | 0 | 0 | unchanged |
| `source/ai/actors` | 65 | 3 | 8 | unchanged |
| `source/ai/actor_combat` | 18 | 6 | 10 | unchanged |
| `source/ai/actor_perception` | 33 | 2 | 9 | unchanged |
| `source/ai/ai` | 13 | 0 | 33 | unchanged |
| `source/ai/encounters` | 39 | 6 | 16 | unchanged |
| `source/ai/ai_profile` | 10 | 0 | 13 | unchanged |
| `source/ai/ai_debug` | 56 | 4 | 0 | unchanged |
| `source/main/debug_keys` | 12 | 0 | 0 | unchanged |
| `source/ai/ai_script` | 62 | 0 | 54 | +24 exact, -1 residual, -23 unwritten |

This is a target-owner gate, not a replacement for the orchestrator's final
full-tree Ninja build and accepted-set sweep after integration.

## Policy and verification

- `tools/fake_match_scan.py` on the three changed source/header files: 0
  review leads.
- Manual focused scan: no `volatile`, inline assembly, optimization pragma,
  `forceinline`, `point_from_line3d`, raw `tag_get`, or raw `object_get` in
  the changed files. The four implemented private helpers have semantic names.
- Focused tooling tests:
  `test_coff_compare.py`, `test_regression_gate.py`,
  `test_fake_match_scan.py`, `test_parked_functions.py`, and
  `test_audit_object_admission.py`: **103 passed**.
- `git diff --check`: no whitespace error (Git reports only the repository's
  LF-to-CRLF checkout warning).
- No `ai_script` entry was added to parked, semantic-match,
  semantic-data-match, or admission-rejection configuration.
- No fuzzy implementation is retained or credited. The remaining 54 target
  owners are honestly unwritten.
- Target-owned `_ai_script_squad_separator` remains absent: data gain is 0.
  The one-byte datum is not fabricated merely to improve accounting.

## Integration order and risk

Cherry-pick the isolated commits in this order:

1. `8b15cc1c2b9c71b301fffc976a1fb9d9a97068f6` — raw Fable provenance.
2. The reconciliation commit that contains this ledger and the measured
   source/header result.

Canonical advanced after the baseline was pinned, so integration may conflict
in `ai_script.c`/`.h` or the four `config/symbols.json` entries. Resolve in
favor of current canonical inherited exact bodies and the semantic private
names, regenerate the split targets, then re-run the final full Ninja build
plus the address-normalized accepted-set regression sweep before pushing. The
`encounters.h` inline-bound correction is the principal full-build risk even
though all eleven direct-includer gates are zero-regression.
