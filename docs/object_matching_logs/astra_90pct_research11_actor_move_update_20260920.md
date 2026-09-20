> Research-only packet11: zero production change and zero exact credit. Latest validated production remains wave9.

# Packet11: actor_move_update — no new supported shape

**CLOSED_NO_NEW_ADMISSIBLE_MECHANISM. Zero source shapes, zero C compiler invocations, zero credit.** All evidence is scratch-only. Production `source/ai/actor_moving.c`, headers, configuration and base object are unchanged at source SHA256 `25b3d9cc2a8881669349be0b774806a8824aea4baef4e821b3bc2bcefa2da4ff`. Frozen HEAD is `55843bda2c9ecb58fe5f1d52facb7e9f575eb1f2`.

| _actor_move_update | Padded bytes | Relocations | Normalized SHA256 |
|---|---:|---:|---|
| January (3,128 meaningful) | 3136 | 72 | ed110da6e891aea013b08551bc9de54482622f7d07fb90e2ca0b7769ac3ebf42 |
| Current | 3088 | 72 | 7231d70bd6875a3ddcc5987178dd71544a7df0c0b5c8d3c62b40a1e6cf256180 |

## History before extraction

Read the fresh-graphs, 100K B/C, 150K w1/w3, 250K w3d and next150 n4 reports, current parks (none for this unit), campaign avoidance packets, and actual September16 `w1_actor_moving.jsonl`/`w14_actor_moving.jsonl` records. The w1 update row records **45 shapes**, with `mu_best_natural.c` and the explicitly blocked `mu_probe_point_from_line3d.c`. The follow-up already tested in-place scale and swapped add operands. Prior natural best was 3136/72 but residual; enabling the inline point body still left two scheduling regions and added a forbidden owner.

Already exhausted: has-path call/reload placement, crouch clear after the vehicle switch, initializer/declaration order, positive timer branch, flag tests, hover nesting/up-vector construction, negate helper, jump order, blend/scale helpers and explicit sum terms. The new task does not make these prior shapes fresh. Avoidance transform forms were not reopened or probed.

## Fresh primary result

Actual read-only Ghidra extraction completed at PE **0x4681a0**; see `ghidra_004681a0.asm/.c`, `ghidra.log` and `collection.json`. Fresh DIA records are `hcea_actor_move_update_sym.txt` and `_lines.txt`. Direct executable reads independently verify the instructions and RTC descriptor in `primary.py`, `primary.json` and `primary-raw.asm`. PE SHA256: `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Its source revision relative to January remains unverified; changed packed flags/layouts are not imported.

- **Actual consumed helper outputs already represented.** At `0x46842d..0x468442`, the avoidance call receives the address of its four-byte emergency float and 12-byte rotation vector. RTC and HCEA name both. Current source has those output objects and consumes them for the same emergency/rotation operations. At `0x469544..0x469552`, the leap helper returns AL, stores one byte and tests it; current `boolean leaped` already captures that result. No new pair of short outputs exists here.
- **Existing widths match.** Raw word stores at `0x4686c6` and subsequent movement-mode arms, field consumption at `0x468766`, and facing word store at `0x46877e` support the current short movement_type/override_facing. Crouch is stored/read as a byte at `0x4687ba/0x4687ce`, then consumed at `0x469250`. No short/long/Boolean type change is proposed.
- **Known blend, confirmed rather than rediscovered.** The raw scale call at `0x4684d2` resolves via `0x404d77` to `0x42e220`. The following four-argument scaled-add call at `0x4684fd` resolves via `0x409ab1` to the previously authenticated `0x42e0d0` body. January +0x209..+0x249 scales all three accumulator components before its component-wise scaled adds. This is the already measured point-helper visibility/ownership boundary, not a new admissible source mechanism.
- **Known control graph.** The later shared crouch clear at `0x468b0c`, path-has-path call, hover nesting and negate helper all occur in the 45-shape history. They were not recompiled. Extra later busy-flag temporaries do not establish a January lifetime or an additional consumed operation.
- **RTC is not a scalar-order census.** Descriptor `0x469640` contains nine protected locals: six 12-byte vectors, one four-byte emergency float and two eight-byte alignment vectors. Fresh DIA corroborates those retained types. It supplies no crouch/movement_type/override_facing scalar local record and no January lexical-scope proof. Decompiler temporary labels and PPC frame/register labels do not fill that gap.

## Exact remaining boundary and preservation

Fresh January/current raw counts are **888/869 instructions**; external relocation multisets agree. These are diagnostic facts, not strict equality or semantic proof. Full target/baseline identities and raw alignment remain in `baseline.json`, `target.asm`, `baseline.asm` and `baseline.alndiff.txt`. The current stream retains the known blend association, argument/store scheduling and crouch write-through differences; the historical allocation-only shorthand must not be read as eliminating the blend discrepancy or proving an arbitrary local change sufficient.

`audit.json` verifies unchanged production source and base object, all **31 exact controls**, 59 function owners, 158 named runtime owner records, 86 non-debug noncode sections, empty COMMON and no emitted point_from_line3d. Whole-object byte identity preserves all owner flags/selection, runtime data and relocations. No candidate JSON or patch is offered.

Reopen only for a new primary-backed consumed value, scalar output/type or real call boundary that plausibly predicts full closure under unchanged ownership. The supplied fresh evidence establishes none. Do not repeat point visibility, crouch/flag/hover/jump, declaration or pressure variants; no missing-file request is implied.

Read-only reproduction: run `collect.py`, `primary.py`, then `audit.py` in this directory. `prepare.py` captures the baseline and should not overwrite it after production changes. Ghidra project is closed; lane ownership released. `IDA_AUTOMATION_UNAVAILABLE`.
