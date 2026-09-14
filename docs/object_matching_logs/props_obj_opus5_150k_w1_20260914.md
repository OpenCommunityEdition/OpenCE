# `props.obj` Opus5 150K house-clean lane, wave w1 (2026-09-14)

## Scope and baseline

- Translation unit `source/ai/props.c`; January target `build/split/source/ai/props.obj`.
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`.
- Baseline real-file gate (`--forbid-emitted-symbol _point_from_line3d`, guard passes): **14 exact / 3 residual / 0 unwritten**.
  - `_prop_add`: residual [size 592 != 608].
  - `_prop_get_active_by_unit_index`: residual [sha].
  - `_prop_get_base_by_unit_index`: residual [size 384 != 400].
- Props has no parks.
- Prior ledgers honoured: `props_obj.md`, `props_obj_old_fable_iterator_reconciliation_20260904.md`, `props_actor_perception_grenade_canonical_integration_20260904.md`.
- `branch_sweep` found no blob with more exact functions.
- Per-function worker blocks: `scratch/workers/ai_script_props_smoothing.md`.

## Result

The final real-file gate is **15 exact / 2 residual / 0 unwritten**, and the guard passes. The row-by-row listing diff against the baseline changes only one row.

| function | padded | relocs | normalized sha16 | status |
| --- | ---: | ---: | --- | --- |
| `_prop_get_base_by_unit_index` | 400 | 15 | `6c8805cdd5bca500` | **newly strict EXACT** |

## `_prop_get_base_by_unit_index`: evidence and source

The old ledger's reopen criterion was "layout evidence for the additional 8-byte lifetime or a newly identified typed scratch object". January's frame supplies that evidence.

### Frame layout

January's frame is `0x4c`. Ours was `0x44`. The January slots are:

| slot | contents |
| --- | --- |
| `-4` | `target_actor_index`, stored in both arms of the swarm/actor select |
| `-8` | actor |
| `-0xc` | unit |
| `-0x10` | never referenced |
| `-0x14` | loop index |
| `-0x4c` | 0x38-byte `actor_position_data` |

### The iterator

The `-0x14`/`-0x10` pair has the layout of `struct prop_iterator` in `props.h`:

- `index` at offset 0;
- `next_index` at offset 4, enregistered in ECX.

The loop code matches the inlined body of `prop_iterator_new` / `prop_iterator_next`:

- the actor re-fetch through `datum_get`;
- `mov [ebp-0x14],ecx; cmp ecx,-1; ...; mov ecx,[prop+8]`.

The same TU's exact `_prop_new_unacknowledged` already uses this iterator. This is the frame-gap law: a missing aggregate local, not a register tie.

### Retained source

- The if/else swarm-controller select.
- The prop iterator loop, breaking on a unit match or a swarm-controller match.
- `result = prop->orphan_prop_index == NONE ? iterator.index : prop->orphan_prop_index;`. January loads the iterator index first and conditionally replaces it with the orphan.
- The acquisition block is unchanged. Its inner pointer is renamed `new_prop`, a name-only change that removes shadowing.

Semantics are identical to the previous body and to the HCEA reconstruction.

## Experiment matrix

| ID | function | shape | size T/B | result |
| --- | --- | --- | --- | --- |
| gb_v1 | get_base | if/else target-actor select | 400/400, frame 0x48 | sha |
| gb_v2 | get_base | + named swarm local | same | inert |
| gb_v3 | get_base | + `prop = NULL` initialiser | same | inert, not kept |
| gb_v4 | get_base | + prop iterator | frame 0x4c | esi/edi mirror |
| gb_v5 | get_base | reuse outer prop pointer | same | inert |
| gb_v6 | get_base | ternary `orphan != NONE ? orphan : index` | registers fixed | arms swapped |
| gb_v7/v8 | get_base | ternary `orphan == NONE ? index : orphan` (+ `new_prop` rename) | 400/400 | **EXACT, landed** |
| pa_v1 | prop_add | no `dead` local, `prop->dead` read back | ABI flip | 4 exact siblings lost |
| pa_v2 | prop_add | `really_dead` from `prop->dead` | 592 | identical |
| pa_v3 | prop_add | HCEA if-form `really_dead` local | ABI flip | 4 siblings lost |
| pa_v4 | prop_add | `dead ? feign == 0 : FALSE` | 592 | sete form |
| pa_v5 | prop_add | `long dead` | 608 | EAX-width ops, sha |
| ga_v1-v3 | get_active | iterator / predicate spellings / break result | 144 | same ECX/EDX mirror |

## Residuals and reopen criteria

- **`_prop_add`** (608/592, 25 relocs). January re-tests AL (`test al,al`) after a deferred `add esp,0x18`. VC7 instead reuses the `and al,1` flags for the `dead &&` branch.
  - Any change to the local count flips the private caller-cluster ABI (unit handle in EAX).
  - Reopen: a form that suppresses the flag reuse without changing `prop_add`'s locals or ABI.
- **`_prop_get_active_by_unit_index`** (144/144, 5 relocs). ECX/EDX mirror between the next-index and the state word.
  - The iterator form did not move it.
  - Reopen: a forced-register anchor or provenance for the state-test spelling.

## Checks

- Point guard passes.
- No parks.
- Owner census against `build/base` and against the real-file baseline object: no new code, data, BSS or COMMON owner.
- `tools/fake_match_scan.py source/ai/props.c`: 0 leads.
- `git diff --check`: clean. The file keeps its CRLF line endings.
- No header, config, ninja, configure or commit action was taken.
