# `ai.obj` alternate-lane packet reconciliation audit

Date: 2026-09-09

Unit: `source/ai/ai`

Donor commit: `482743d8ab1af7af2811e286deee6d9fd9550b50`

Canonical revision audited: `a565219da15d4c5360c6021714429573b49e95fb`

## Disposition

The donor packet is already fully represented in canonical source and must not
be replayed or counted again. Although donor commit `482743d8a` is not itself
an ancestor of canonical, its `source/ai/ai.c` blob is
`9b95305ac6f325931172ae3682a7c57b50db7188`, exactly the same blob introduced
by ancestral replay commit `1a6da50d5`.

Canonical subsequently applied three evidence-driven corrections:

- `44a36bec9` restored January-authenticated editor names and control choices;
- `603f5b41c` rejected the unsafe exact `csmemmove` byte-count bug and parked
  `ai_handle_editing` at zero credit;
- `49417c060` moved the complete `ai_profile` declaration to its true owner,
  `ai_profile.h`, and initialized the line-of-sight collision fraction before
  every possible fog-classification read.

Those corrections are preferable to the older donor snapshot. Importing the
donor's translation-unit-private partial `ai_profile` structure, uninitialized
local, or unsafe editor copies would regress the house rules and runtime
correctness. No source or header change is therefore required by this audit.

## Strict exact state

The production source gate reports **41 exact / 4 residual / 1 unwritten**.
The five owners originally contributed by this packet remain strict exact:

| owner | meaningful bytes | padded bytes |
| --- | ---: | ---: |
| `_ai_place_pending_mounted_weapons` | 288 | 288 |
| `_ai_generate_line_of_fire_pill` | 96 | 96 |
| `_ai_find_line_of_fire_friend_pills` | 368 | 368 |
| `_ai_update` | 208 | 208 |
| `_ai_handle_spatial_effect` | 1,130 | 1,130 |
| **historical packet total** | **2,090** | **2,090** |

This reconciliation audit adds **zero new exact functions and zero new exact
bytes** because all five owners were already admitted. Every inherited exact
owner remains exact. `_ai_disconnect_from_structure_bsp` remains the one
unwritten target and is outside this donor packet.

## Honest fuzzy retention

The following complete, semantically credible bodies remain non-exact and
receive zero exact-function and zero exact-byte credit:

| owner | target/base padded bytes | target/base relocations | objdiff |
| --- | ---: | ---: | ---: |
| `_ai_test_line_of_fire` | 336 / 336 | 10 / 10 | 88.16964% |
| `_ai_test_line_of_sight` | 1,008 / 1,040 | 50 / 50 | 97.97885% |
| `_ai_test_ballistic_line_of_fire` | 944 / 944 | 49 / 49 | 94.50000% |
| `_ai_handle_editing` | 1,296 / 1,296 | 70 / 70 | 93.23018% |

Current normalized target/base SHA-256 fingerprints are:

| owner | target | base |
| --- | --- | --- |
| `_ai_test_line_of_fire` | `0aef44a538a12480aec2d0e85e44b434a56a3a3703ca0bc324a00dfdad2a3b3c` | `a2c988c9fb9cb73a850cc3b148527a6ed9891b5a4425d9eef7a4babc2851cf25` |
| `_ai_test_line_of_sight` | `aecb662c1c4868e27cbd680dba829bdedc994fc2f91911172ad86ecdfdec9b86` | `61a3ce7f829fcafe2bf34705e9d6e2a0bb2be0d80257c1921cc134edb6eb0488` |
| `_ai_test_ballistic_line_of_fire` | `6c90076033e91f26356e42166cf4bb0a8d5a902ae35bc67db15aec19ee9982ac` | `c8f5527aacb22c833089881ef848f5d4056908d6524c3be7ac4099325db85bce` |
| `_ai_handle_editing` | `652338b1724b0f70d82160cd72c21e568a24fd099c4208e692215bb4e3d93cf8` | `5a0356c0457d3b500449b2cb4abfeeee8d281c857e47b9eb1521e35d63dbf5fd` |

The retained bodies use project types, named collision flags and AI enums,
typed actor/prop/unit/tag accessors, semantic private names, multiline
parameters, and explicit terminal returns. The visibility helper declarations
remain with their math and collision owners. `ai.c` opts into the required
external calls locally, preserving the January inline schedule without
emitting a `_point_from_line3d` COMDAT.

The two `#pragma optimize` scanner leads tightly bracket only
`ai_handle_spatial_effect`. January's function-local `/Od` signature and the
strict 1,130-byte, 47-relocation equality justify that narrow compiler-mode
boundary; no unrelated owner is inside it.

## Provenance review

- The donor ledger
  `docs/object_matching_logs/ai_obj_large_visibility_spatial_packet_20260909.md`
  and canonical batch ledger
  `docs/object_matching_logs/lane_reconciliation_10k_batch_3_20260909.md`
  agree on the five exact owners and 2,090 historical bytes.
- The Opus donor at `eeedd72e5ba0fc1761519f68cad241fe606320e7`
  supplied the earlier object/event foundation documented by
  `ai_obj_opus_object_event_reconciliation_20260909.md`; its admitted subset is
  already in canonical through replay commit `02efd8870` and later work.
- The recent Fable branches contain no later independent `ai.c` packet beyond
  their already-reconciled shared history.
- HCEA cross-build maps under
  `research/halocea/src/ai_generate_line_of_fire_pill.c`,
  `ai_find_line_of_fire_friend_pills.c`, `ai_test_line_of_fire.c`,
  `ai_test_line_of_sight.c`, `ai_test_ballistic_line_of_fire.c`,
  `ai_handle_spatial_effect.c`, `ai_handle_editing.c`, `ai_update.c`, and
  `ai_place_pending_mounted_weapons.c` independently support the retained
  control flow and semantic names. January COFF remains the exactness
  authority.

## Verification

The before and after production-source command is:

```text
python tools/campaign/gate.py source/ai/ai --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/ai-reconcile-final.obj
```

Both snapshots report `exact 41`, `residual 4`, `unwritten 1`; the emitted
symbol guard passes. No `config/parked.json`, `config/symbols.json`, AI Debug,
shared header, build configuration, or unrelated source file is changed by
this audit.
