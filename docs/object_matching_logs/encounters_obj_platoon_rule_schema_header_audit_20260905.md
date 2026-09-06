# Encounters object: platoon-rule schema header audit (2026-09-05)

## Scope and disposition

At root commit context `4f90c05`, this ledger records an independent frozen
all-section audit of the one platoon-rule schema packet: the original ten rule
enums, the 8-byte `platoon_rule`, embedded fields at `+0x30` and `+0x3C`, the
remaining opaque bytes, and the required `main/console.h` include in
`encounters.c`.  The audit made no production, header, park, build, gate, or
configuration change.  Active Fable work was excluded.

**Result:** all 13 captured translation units preserve runtime sections,
program symbols, definition state, COMMON records, and the existing point
helper.  No unit requires review.  The only differences are seven sections
whose relocation/symbol records differ solely in compiler-local `$L...` label
spellings.  No initializer or speech-timer exception was reused.

## Immutable inputs and method

- Before inventory:
  `scratch/encounters-platoon-schema-before-20260905.json`, SHA256
  `A83C83D90860BD4233AAAA3D43A2E82791E93B74B02F6B0B59F99880B382EEBC`.
- After inventory:
  `scratch/encounters-platoon-schema-after-20260905.json`, SHA256
  `8A4E02D812D68F8C8A1EADF8A38F882919FA2F45A5D9D9878C836AEA20A10DAB`.
- Header hashes recorded by those inventories:
  `5424E4E887430E9DEF8E23DDE5B1F7C02DA0662A11F31A5BD80A40E1FD460618`
  before and
  `52E4B251E63CB58D9C8DCF5BC86EBEBA1852F473CECEBEA06354FDA1864783C5`
  after.
- `encounters.c` hashes recorded by those inventories:
  `20143A0B6DA455DA20B1336E5BD6F2F7F7EA1171B531D45E5AA9382EEA055828`
  before and
  `15244DFAB798ED63746FA3689CD1CFF69AF35BBA995B2F7661E055504806224B`
  after.
- All-section implementation:
  `scratch/observer_dependents_runtime_audit_20260905.py`, SHA256
  `76D1978E49E62CFD7B6734EE578644729857E92158C533036B8777AB7B783F50`;
  its nine synthetic invariance/negative checks passed before measurement.
- Supporting immutable implementations:
  `tools/regression_gate.py` SHA256
  `299F15D1569096730508C85EEE9271BA4798C8E4F42F51BDCCB89F1C1D84EA3F`
  and `tools/coff_compare.py` SHA256
  `22F8E11C7B5B5B1E1CB9BC51D2E48C0FA4912DEF9C1D48BD09DB80735EE8204E`.

The audit tool derived the 13-unit list and every object path from the paired
inventories, required exact phase/list/key/path agreement, verified each
recorded object hash, and bracketed parsing with a second hash.  It called the
Observer library's `fingerprint` and `compare_unit` functions directly rather
than its historical 46-unit main program.

## Complete classification

- Sections: `1890 -> 1890`; 1,883 exact and seven proven
  compiler-local-label-spelling-only; zero runtime or relocation-metadata
  review records; zero runtime/debug additions, losses, or other changes.
- Program symbols: `5082 -> 5082`; 138 raw removals and 138 raw additions are
  compiler-local labels.  After label-only normalization, the delta is `0/0`.
- Defined functions: `911 -> 911`, with zero additions or removals.
- Undefined externals: zero additions or removals.
- Undefined/common/defined/absolute state transitions: zero.
- COMMON: `15 -> 15`, exactly unchanged.  The records are the five
  `ai_debug` commons (`_global_ai_debug_drawstack_next_position`,
  `_global_ai_debug_drawstack_last_position`, `_ai_debug`,
  `_global_temporary_render_color`, `_global_ai_debug_drawstack_height`), two
  in `ai_script` (`_object_list_data`, `_object_list_header_data`), four in
  `encounters` (`_pursuit_data`, `_squad_array`, `_encounter_data`,
  `_platoon_array`), `_player_data` in `players`, and three in `hs`
  (`_object_list_data`, `_object_list_header_data`, `_hs_syntax_data`).
- Point helper: the sole `_point_from_line3d` record remains in `ai_debug` with
  owner `.text|owners=_point_from_line3d`, storage `2`, type `32`, value `0`;
  no point record was added or removed.

The seven label-only owners are:

- `source/ai/actors`:
  `.text|owners=_actor_handle_unit_effect`.
- `source/ai/ai_communication`:
  `.text|owners=_ai_communication_started`.
- `source/ai/ai_debug`:
  `.text|owners=_ai_debug_render_actor` and
  `.text|owners=_code_00039e80`.
- `source/ai/encounters`:
  `.text|owners=_encounter_create`.
- `source/game/players`:
  `.text|owners=_player_examine_nearby_objects` and
  `.text|owners=_player_handle_action`.

The root-supplied wider validation context, not rerun by this read-only audit,
was full Ninja PASS, stable `6219/8245` unchanged, and 282 valid parks.

## Durable evidence

- Reproducer:
  `scratch/encounters_platoon_rule_schema_runtime_audit_support_20260905.py`.
- Summary:
  `scratch/encounters-platoon-rule-schema-runtime-audit-support-20260905/summary.json`,
  SHA256
  `AFE707F4CA30E8CFB9167C63EE6947DDF47B49951C2CE419864219B40CB16A94`.
- The summary links 13 per-unit reports and their SHA256 hashes.  Each report
  retains both complete fingerprints: every section flag, alignment, COMDAT
  selection, normalized payload, relocation target, symbol storage/type/value,
  raw symbol delta, COMMON delta, and definition-transition evidence.

Disposition: this exact header/include packet is independently runtime-clean
under the frozen 13-unit all-section comparison.  This finding grants no
blanket exception to any later body or schema packet.
