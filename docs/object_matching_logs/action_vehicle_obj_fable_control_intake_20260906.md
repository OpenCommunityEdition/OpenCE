# ActionVehicle Fable independent intake (2026-09-06)

## Decision

The smallest house-rule-clean packet is **`_action_vehicle_control` only**.
Keep `_action_vehicle_begin` held.

The control packet is three production-source edits for a later owner trial:

1. Add the donor control body and its TU-local
   `_action_vehicle_idle_look_type = 4` enum to
   `source/ai/action_vehicle.c`.
2. Add `void action_vehicle_control(long actor_index);` to the genuine owner
   header, `source/ai/actions.h`, under an `ACTION_VEHICLE.C` prototype block.
3. Remove the now-redundant consumer-local declaration of
   `action_vehicle_control` from `source/ai/actions.c`.

Do not copy the whole donor file: it also contains the held begin body. No new
header include, symbol alias, macro, global, static helper, data owner, or
configuration change belongs in the control-only packet.

`_action_vehicle_begin` is held because its HCEA/prior-rejected contract
already had the same three semantics: reset the counter, store
`game_time_get()`, and copy `body_position`. Fable's only material source-shape
change is the `struct vehicle_state_data *state_data` alias. Exactness therefore
comes from a new lvalue topology, not new semantic provenance, and does not
satisfy the explicit 2026-08-21 reopen condition. If begin is ever lawfully
reopened, it additionally needs the genuine `#include "game/game.h"`; the
genuine public declaration of `game_time_get` is `source/game/game.h:171`.

## Why control is a genuine semantic reconciliation

The later-build HCEA source uses the wrong January priorities:

- lock-facing: `_primary_priority_exact_facing` (canonical value 2);
- path-present: `_primary_priority_facing` (canonical value 1).

The January target machine code instead writes 4 and 3. Canonical
`source/ai/actors.h:62-66` authentically names those values
`_primary_priority_locked_aiming` and `_primary_priority_aiming`. The Fable
control body used those two names in its literal first saved source, before any
candidate object existed. This is a code-proven semantic correction to the
HCEA contract, not an alias-only scheduling retry.

The remaining contract is also owner-backed:

- `actors.h:210-214` owns `_direction_specification_movement = 0` and
  `_direction_specification_vector = 4`;
- `actors.h:265` owns `actor_get`, `actors.h:1322` owns `actor_data`, and
  `actors.h:1157` owns the `actor_path_has_path` prototype
  (`source/ai/actor_moving.c:310` defines it);
- `actions.h:246` owns `struct vehicle_state_data`, and its action-data union at
  `actions.h:389` owns the `vehicle` arm;
- the schema has `lock_facing` at vehicle offset 44 and
  `destination_facing` at offset 60. January reads actor offsets `0xC8` and
  `0xD8`, fixing the vehicle arm base at `0x9C` and independently confirming
  both fields;
- `actors.h:582` owns the short `idle_look_type`. The value 4 is present as a
  TU-local action constant in both `source/ai/action_charge.c:127` and
  `source/ai/action_avoid.c:35`; no shared idle-look enum exists.

The target control has 175 meaningful / 176 padded bytes and three
relocations: `DIR32 _actor_data` at `+4`, `REL32 _datum_get` at `+17`, and
`REL32 _actor_path_has_path` at `+89`. Its normalized SHA-256 is
`81612c85cf900b15c2de8ea6d52646f5727a96fbea5e2f13018e6f9613fcd293`.
The first code-producing Fable candidate and committed donor object are strict
equal on all bytes and relocation identity.

Both target and candidates define `_action_vehicle_control` as external
function symbols (`storage=2`, `type=0x20`, section-relative value 0). There is
no private/static implementation symbol to rename.

## Literal-first versus first-code-producing provenance

The Fable report's phrase "EXACT - first compile" is accurate only when
"compile" means the first code-producing compile:

- `scratch/_gate_16992.c`, created `2026-09-04T22:46:23.2697770Z`, is the
  literal first saved source. It already contains the final state alias,
  locked-aiming/aiming branches, direction choices, and tail stores, but uses
  undeclared `_idle_look_combat`. No `_gate_16992.obj` is preserved.
- `scratch/_gate_35296.c`, created
  `2026-09-04T22:46:38.1663797Z`, differs only by adding the TU-local
  `_action_vehicle_idle_look_type = 4` declaration and renaming that one use.
  `_gate_35296.obj` is the first preserved code-producing object, and control
  is strict exact there.
- The parse/name repair is disclosed. It did not change control flow, field
  access, priorities, call identity, or emitted runtime data, and is accepted
  here as pre-code-generation semantic-name closure backed by the two sibling
  owners above. It must not be described as literal-first-source exact.
- `_gate_35296.c` and the committed donor `source/ai/action_vehicle.c` are the
  same Git blob, `0e89db5e7b3072e1ebb5220f7d5a436428b57d69`.

The earlier canonical ledger is not superseded silently. It records the
one-shot HCEA control miss as normalized SHA
`3f96cbf0cdc3a58d1013ac2d44df261acc0d3b68c37ebab639956ddfa1a046ad`
against the January hash above and requires new authenticated contract
evidence. The 4/3 priority correction is that new evidence for control. No
equivalent semantic delta exists for begin.

## Caller and owner closure

The complete current source search finds only one consumer:

- `source/ai/actions.c:764-765` declares control locally;
- `source/ai/actions.c:967` stores its address in the vehicle action callback
  table.

Current base `actions.obj` carries one `DIR32` reference at `.rdata+536` to an
undefined external function symbol `_action_vehicle_control`; January split
`actions.obj` carries the same reference at `.rdata+536`. The corresponding
begin reference is `.rdata+524`, but begin remains out of scope. No other
current base or January split object contains either callback symbol.

`actions.c` already includes `actions.h`, so moving the control declaration to
the genuine header and removing the consumer-local declaration leaves the
same signature visible at the callback table. Adding a new `action_vehicle.h`
would be an artificial facade and is not admissible. Leaving only a C-local
declaration is likewise not owner closure.

Because `actions.h` is shared, a later trial must freeze and compare all of its
actual consumers plus `actions.obj` and `action_vehicle.obj`; this intake did
not query Ninja or compile anything.

## COFF delta

Against the current canonical base object, `_gate_35296.obj`,
`ng_action_vehicle.obj`, and the donor final object all add exactly two named
external code owners: begin and control. For the proposed subset, admit only
control.

All seven inherited exact owners remain exact:

`_action_vehicle_update`, `_action_vehicle_flush_structure_indices`,
`_point_from_line2d`, `_distance_squared2d`, `_distance2d`,
`_negate_vector2d`, and `_object_get_bounding_sphere`.

The candidate has no COMMON symbols, BSS section, new writable/read-only data
owner, or named static code helper. Its only additional undefineds are
`_actor_path_has_path` and `_game_time_get`; the latter belongs solely to the
held begin body. Section-symbol `.text` entries with storage 3 are compiler
section bookkeeping, not private helpers. The local enum emits no symbol or
storage.

## Pinned artifacts

Canonical frontier: commit
`c2561af50c7fda9c5ea3326b35b4ba10f90d17f5`.

- January target:
  `build/split/source/ai/action_vehicle.obj`, 11,826 bytes,
  SHA-256 `49F00A0115CECF578E51102FC33F665F876A4FAFCB78EE3B53520545406EF769`.
- Current canonical candidate baseline:
  `build/base/source/ai/action_vehicle.obj`, 3,084 bytes,
  SHA-256 `84FE91F3F197946CA8EC1EED569AFBF77774B44B6F2CEAE4A8540396979E4120`.
- Current callback consumer:
  `build/base/source/ai/actions.obj`, 56,841 bytes,
  SHA-256 `05FB549A9F707E1E97EE9A44D11CF9AE7AC534752A00ECF22652C60C3374D9D4`;
  January `build/split/source/ai/actions.obj`, 53,049 bytes,
  SHA-256 `624BB4506ADFF68E9888A20E44042F28EC7F7E4FDC33115C64C25BEB1270B5B5`.
- Canonical source:
  `source/ai/action_vehicle.c`, SHA-256
  `22560B33AD7A0250E1ADBD5F500C52DF4102FEAC97E2A280E1E2D1F29428529F`,
  Git blob `6f689f90311fc56f1f3c0d58c593df81390efdca`.
- Canonical owner header:
  `source/ai/actions.h`, SHA-256
  `EA1BEC48DFCC5BDAD4DC01137E8C9D4A1F6AA878EE6F3EAEF4D0EF13E64AEE62`,
  Git blob `5df907fab02c19fd14904ede814923362e2309f5`.
- Canonical consumer source:
  `source/ai/actions.c`, SHA-256
  `F6347EB2573A36F12CF0C9253DA66504441B8743269E8847FD3866770571D7EC`,
  Git blob `b1f6be227e232de1a7a5dec92710b5dd8aaea3e6`.
- Canonical schema/API source:
  `source/ai/actors.h`, SHA-256
  `411A70414C9ED5605A6AB01D14503D7E4B82F2B04BE1D12865CD6D96F45D0B8A`,
  Git blob `ea715dbedf141721eb4c1bf4585bab5cb7c905be`.
- Main prior ledger:
  `docs/object_matching_logs/action_vehicle_obj_jonas_one_leaf_20260821.md`,
  SHA-256 `C01350FD8E5C4CBAF48026405D7480E2297FAB4CC307BA0D322693E931627372`.
- HCEA begin source:
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/blam/ai/action_vehicle_begin.c`,
  SHA-256 `4820BFAFAACD281D4F1DBA31312158B158AF55AAD5DAFF4D04E98D247763A3B0`.
- HCEA control source:
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/blam/ai/action_vehicle_control.c`,
  SHA-256 `96F8D95AA428D7AA9F1535A4C2847B8CDA6D97F8C7467E02B883B2C1924C3716`.

Donor frontier: commit
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`, parent
`96086c9c4c187dd539af13bdaa9252d6e46ad8f6`.

- Donor report: `scratch/reports/action_vehicle.md`, SHA-256
  `36D66D460BD2A3F7E0D7C1C623F76A580BA012B22E979AA0A8226E3E3A4043C6`.
- Donor facts: `scratch/facts/action_vehicle.md`, SHA-256
  `24D06EEAC322DE860653F348308B302404CEEF1B0B9DD31C4C7D6A927DAC052F`.
- Literal first source: `scratch/_gate_16992.c`, 4,808 bytes, SHA-256
  `5300FDEB147AA82B39298432663C63F4D7D8E12452B36B386524F7849F7D0F8D`,
  Git blob `ea342b9e03c72d12584831d6fef4f2a95d3c8f99`.
- First code-producing source: `scratch/_gate_35296.c`, 4,869 bytes,
  SHA-256 `F8F6B69F62B253478B494FE68AF1DABBE987175660E0C41C976EA23F54266F99`,
  Git blob `0e89db5e7b3072e1ebb5220f7d5a436428b57d69`.
- First code-producing object: `scratch/_gate_35296.obj`, 3,651 bytes,
  SHA-256 `3AAC2DB9C10B2A07E9614358D93F416E5C1FBC7B8EAAD75EA355BF10F6C8E66B`.
- Saved later natural object: `scratch/ng_action_vehicle.obj`, 3,651 bytes,
  SHA-256 `702BFF861ACA3E3D961157DCDBD7D7196A812D5F5EEA3CDFD5A06F9EDD721431`.
- Committed donor source: `source/ai/action_vehicle.c`, SHA-256
  `3E5997DDB9769E1E80682B21E117986BACC2DD1BFD238877ACA392F3A8990A7F`,
  Git blob `0e89db5e7b3072e1ebb5220f7d5a436428b57d69`.
- Donor final object: `build/base/source/ai/action_vehicle.obj`, 3,667 bytes,
  SHA-256 `0736AE24E228A91848D26B73F8FE86D72EC9B7ACEA9FF42ACA9337D349E4D207`.

The differing raw candidate-object hashes are explained by producer/debug
record paths and timestamps; the hardened function comparison is strict equal
for both proposed functions and reports zero inherited losses.

## Scope statement

This was read-only over canonical production files and the donor. No source,
header, configuration, object, Ninja state, or donor file was changed; no
compile, configure, Ninja, or commit command was run. This report is the only
new file.
