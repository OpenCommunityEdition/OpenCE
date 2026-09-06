# Encounters initializer-cluster runtime review (2026-09-05)

## Result

The first natural candidate passes **49/49 integrated differential cases**
against January. The real public `encounters_initialize_for_new_map` caller,
private `encounter_new`, and private `squad_reset_starting_locations` execute
together. Both header accessors and the original `__ftol2` execute real object
code, rather than being replaced by behavioral stubs or artificial private
calling conventions.

All compared external API calls, arguments, ordering, assertion text/path/line,
allocation handles, exit behavior, and complete runtime-memory regions agree.
Every nonfatal case additionally agrees with an independently expressed
high-level state model. Tags stay unchanged; surrounding memory guards, stack
balance, and public-call callee-saved registers are preserved.

This is bounded behavior evidence, not whole-game equivalence or byte/owner
admission. It changes no production source/header/configuration and awards no
code/data/Matching credit. Root owns the separate exactness, header-blast,
runtime-owner, ordinary-link, park, and complete-build checks.

## Frozen artifacts and reproduction

Canonical root:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`.

| Artifact | SHA256 |
| --- | --- |
| `build/split/source/ai/encounters.obj` | `7edc1863eb2c08f7f2ad612e3cfa7b246c0eb9a3b5907fd08c2c17ecd9a056a3` |
| `scratch/encounters-initialize-cluster-first-natural-20260905.obj` | `4d921a37f50e65fb8fdde426d9a93b8b4d208d5de35ee77e8d8c992578cd8649` |
| `scratch/encounters_initializer_cluster_behavior_20260905.py` | `4c14c753a8b509d63a30284cd4ea0e02760d6b86d97e2c8be582b6c17f86771a` |
| `scratch/encounters-initializer-target-behavior-20260905.json` | `57546e98c9de6c06ba13043121d121e8f189d577fb115f21c9173888c157bc18` |
| `scratch/encounters-initializer-first-natural-differential-20260905.json` | `3dfeffeb033094929db561a46d876b3f865b88ea45fc5251ab484a8ca5f52a95` |

Run:

```text
python -B scratch/encounters_initializer_cluster_behavior_20260905.py --candidate scratch/encounters-initialize-cluster-first-natural-20260905.obj --output scratch/encounters-initializer-first-natural-differential-20260905.json
```

The first target-only harness was tested before receiving a candidate object.
Its allocator model was then tightened, still before candidate execution, to
match the real data APIs: `data_make_valid` clears datum identifiers only,
`datum_new` zeroes the allocated 0x74-byte datum and sets its identifier, and
the seed from `"encounter"` produces 0xEE65, incrementing per allocation.
These facts are independently in `source/memory/data.c`'s `data_make_valid`,
`data_delete_all`, `datum_new`, and its allocation-initialization helper.
The final target-only report above passes all 49 cases. No candidate source
was altered in response to this harness and no source-shape variant was tested.

## Actual executed helper ownership

The loader follows ordinary COFF relocations recursively. A candidate-local
header helper is used if actually emitted; otherwise the January cross-object
owner is used. The fallback artifacts are:

- `_encounter_get_squad`: `build/split/source/ai/actions.obj`, SHA256
  `624bb4506adff68e9888a20e44042f28ec7f7e4fdc33115c64c25beb1270b5b5`.
- `_encounter_get_platoon`: `build/split/source/ai/ai_script.obj`, SHA256
  `418dee372c43ec72c398032311f1dd94a3abcb90806860249b3b59bfc29c14f1`.
- `__ftol2`: `build/split/libs/libcmt/ftol2.obj`, SHA256
  `e5999a9cc2e8866130d3b47e1dda136eeae156acf2d719434502577f72df51c3`.

Reading the January split AI Script object is target evidence, not inspection
of the active Fable source/tree. No active Fable work was inspected or changed.

The loader resolves actual defined strings/constants from their object
sections. An external January string/constant uses its authenticated symbol
offset in the unchanged `cachebeta.exe`; it is not substituted with a generic
assertion string. Unknown runtime owners or relocation kinds fail closed.
The real x87 conversion is exercised with control word 0x037F; it is not
approximated with a host-language conversion stub.

## Modeled outside APIs

The only behavioral hooks are genuine external APIs: `global_scenario_get`,
`data_make_valid`, `csmemset`, `datum_new`, `datum_get`,
`tag_block_get_element_with_size`, `csprintf`, `display_assert`, and
`system_exit`. Their corresponding external data slots supply the scenario,
encounter/pursuit arrays, squad/platoon arrays, and temporary formatting buffer.
These are harness-only hooks, never compiled production stubs.

Tag access validates the complete block identity, element index, and required
stride before returning an element. Datum lookup requires the complete salted
handle returned by allocation, catching low-word truncation. Allocation failure
is terminal for later allocation attempts, modeling exhausted storage without
inventing recovery that could violate scenario-index correspondence.

Runtime data includes all 1,024 squad slots, all 256 platoon slots, and 128
independently placed encounter datums. Initial poison outside API-owned
initialization exposes unintended writes. Entire buffers and 64-byte exterior
guards are compared, not just the fields expected to change; tag input bytes
are checked unchanged. These synthetic addresses are test-fixture addresses,
not recovered source fields or production raw-offset access.

## Coverage

The 49 cases cover:

- Zero encounters, an empty encounter, allocation failure initially and after
  a successful encounter, and three mixed encounters with advancing bases.
- Starting-location counts 0, 1, 31, and 32, each with no required locations,
  edge-required locations, and all locations required. Required bit 31 is
  included, and the unused bank remains all ones whenever the count is nonzero.
- All eight combinations of the three consumed encounter flags, including
  ordinary and NONE team values.
- Every combination of squad delay-forever/automatic-migration flags, plus
  platoon defending/nondefending values with unrelated low/high bits.
- Finite negative, zero, fractional, and positive delay values, using the real
  x87 multiply/conversion. Delay-forever remains the authentic 999 sentinel.
- Inactive respawn, negative min/max, independently positive min and max,
  zero-total unlimited 999, positive total, negative total, and signed-short
  maximum total.
- Exactly 64 squads / 32 platoons in an encounter; exactly 1,024 squads /
  256 platoons across encounters.
- All four original constructor overflow assertions: squad-per-encounter
  line 1444, squad-per-map line 1448, platoon-per-encounter line 1483, and
  platoon-per-map line 1487. The complete pre-exit state and diagnostic event
  sequence match in these cases, too.

The bound is the accepted valid-tag domain **0 through 32** documented in
`encounters_obj_platoon_schema_starting_location_bounds_20260905.md`. Count 33
is intentionally not counted as an equivalence/admission case; January's
known overwrite beyond a one-word bank is already documented separately.

Not covered: arbitrary corrupt tag pointers, negative block counts, actor
starting-location counts above 32, NaN/infinite delay conversion, allocation
recovery after exhaustion, arbitrary data-array corruption, or complete game
startup. No conclusion for those cases is implied by the 49/49 result.
