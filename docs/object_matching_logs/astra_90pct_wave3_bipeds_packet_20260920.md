# Bipeds next-wave evidence packet

Worker scope is `source/units/bipeds.c`, prioritizing `_biped_update_physics` and `_biped_update_moving`. The baseline at packet start is `905e5e3b41591af98c82accf3f57fc38118c2e7f`; all work is scratch-only. No production source/header/configuration, Ninja, or commits were changed by this worker.

Fresh baseline gate: **42 exact, 9 residual, 0 unwritten**. Physics is 5,248 bytes / 112 relocations versus January 5,376 / 124. Moving is 4,000 / 97 versus January 4,080 / 97. Existing exact siblings are controls. The point helper remains externally owned through the existing `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` mechanism; a surplus `_point_from_line3d` definition is not admissible.

## Current review proposal: restore the omitted diagnostic capture

`scratch/astra-bipeds-evidence/bipeds-debug-block.c` and `.patch` add the canonical collision-debug header include and the missing diagnostic block after the collision-skip/collision-move merge. `bipeds-debug-block-review.json` records source hash, raw evidence, type provenance, and final scratch validation. The initial required root-owned header proposal was `collision_debug-header.patch`; root subsequently supplied its final dependency-complete header and HS migration.

The block is a separate source omission. January target instructions at `_biped_update_physics+0x08c3..0x0940` directly prove:

| Operation | January evidence |
|---|---|
| Test `debug_biped_physics` | byte load `+0x08c3`, relocation `+0x08c4`; false branch to `+0x0940` |
| Require an object owned by a player | `object_get_and_verify_type(physics->biped_index, -1)` at `+0x08d1`; load embedded object's `owner_player_index` at `+0x70`, compare `NONE` |
| Capture input position | three stores to `collision_debug_point+0/+4/+8`, `+0x08ea..0x0901` |
| Enable repeat | byte store 1 at `+0x090d` |
| Capture input velocity | three stores to `collision_debug_vector+0/+4/+8`, `+0x0914..0x091f` |
| Set length | float 1.0 bits `0x3f800000` at `+0x0925` |
| Set width / height | fields `physics+0x58` / `physics+0x54` at `+0x092f..0x093b` |

Fresh actual Ghidra assembly, `ghidra_008c9b80.asm` at `008cae39..008caedb`, independently corroborates the predicate, all six globals, and source order `repeat`, `point`, `vector`, `length`, `width`, `height`. Ghidra's executable is the 2020 symbol build with SHA256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Its same-source-revision claim is unauthenticated; January independently authenticates this specific graph. The later object's owner field is at `+0x7c`, so that numeric offset was not transplanted into January source.

The capture deliberately reads **pre-collision `position` and crouch-adjusted `velocity`**, not `clipped_position` / `clipped_velocity`. It executes after either collision branch. This preserves the diagnostic's purpose: retaining the tested pill sweep for repeated rendering.

The final candidate uses `object_get(physics->biped_index)->object.owner_player_index` and `1.0f`. `object_get` is the existing canonical typed owner accessor in `objects/objects.h:188`, wrapping `object_get_and_verify_type` with `_object_mask_all`; it preserves January's all-object mask and the embedded object field. An interim suggestion to narrow the mask was withdrawn after the root inspected the actual macro. The historical `best_collision_index != NONE` guard remains; the target's `collisions[-1]` read is not reproduced. No exact credit is requested.

## Header and storage evidence

The six proposed declarations are:

```c
extern real_point3d collision_debug_point;
extern boolean collision_debug_repeat;
extern real_vector3d collision_debug_vector;
extern real collision_debug_length;
extern real collision_debug_width;
extern real collision_debug_height;
```

`collision_debug.c` already supplies typed extern declarations for point/repeat/vector and definitions for length/width/height. Independent owner review by `source_context` confirms January's point/vector/repeat storage belongs to `linker_common.obj`, BSS sections 76/75/93 with sizes 12/12/1; no new definitions are proposed. The three dimensions belong to `collision_debug.obj`, initialized data+8 (`100.0f`) and BSS+0x7118/+0x711c (`0.0f`). January's access widths corroborate 12/1/12/4/4/4 bytes.

The original header lacked real-math type dependencies. The concrete proposal identified `math/real_math.h`; root's final header supplies both cseries and real-math dependencies before the typed declarations. This dependency and all affected C consumers belong to the root's full-build review, including the HS byte-array façade migration. No consumer-local replacement prototypes were added. The final diagnostic candidate was compiled only after the root made the real shared header available.

Final scratch gate: **42 exact / 9 residual / 0 unwritten**, emitted-point guard passed. `_biped_update_physics` is now **5,376 bytes / 124 relocations**, matching January's extent and relocation count, with SHA-only residual. The entire symbolic relocation census equals January. Candidate normalized SHA is `bf945c9301d7848f33e0c5a7765f3b3d820380a2310a576d768b2e7fd0968b64`; target is `f202fb20dd4d8fba5d969bf881249cedf6de9678d56d207ada9f048ba0f44098`.

Only the physics owner differs from baseline. No code owner was added or removed; `_point_from_line3d` remains absent; all 42 exact controls and all other residual owners are unchanged. Moving remains exactly baseline 4,000 / 97 with normalized SHA `c457c69052f0b2821c44a579427ff4038c9101632d9de17670ccd7acdba9fe6a`. COMMON symbols and production data are unchanged. The only noncode difference among 121 sections is compiler `.debug$S` object-name metadata. `review_final.py` reproduces this audit, and `bipeds-debug-block.gate.txt` is the complete gate receipt.

Fresh DIA `-sym` queries for these six data names returned empty output, so they are explicitly **not PDB type receipts**. Named function/local/line exports succeeded for both biped functions. Existing typed declarations, independent January storage ownership, and primary target accesses supply the global evidence.

## Fresh tools and bounded local evidence

`scratch/astra-bipeds-evidence/collect.py` ran actual Ghidra read-only through `AstraBipedsLaunch.java`, opening the existing project with `getReadOnlyDomainObject` and no transaction/save. It freshly disassembled and decompiled PE entries `008c5840`, `008c7c50`, and `008c9b80`; all three completed successfully. `ghidra.log` records executable SHA and success. The launcher also emitted `bipeds_source_xrefs.tsv`.

RTC has 39 protected aggregate records for physics at `008c9b80`. They distinguish the 3D/2D acceleration branches, the four input/output position/velocity aggregates, 16 collision planes, support/adjacent/ground planes, and the standing feature test. The moving correspondence `008c5840` has 12 protected aggregate records, including 204-byte `physics`, 244-byte player-information copy, movement and collision results; HCEA corroborates this distinctive combination. The nearby `008c7c50` has only six records and was not substituted for moving. Complete records are in `bipeds-rtc.json`.

Fresh HCEA exports are `hcea_biped_update_physics_sym.txt`, `_lines.txt`, and the corresponding moving files. Physics is `017AEF08`, signature `static void(struct biped_physics *)`; moving is `017B2FC8`, `static void(long, struct unit_animation_update_data *)`. Lines identify `d:\projects\code\hcex\sources\units\bipeds.c`, MD5 `026D0F6BE380914B377B1BF4A1CE0DAE`. HCEA is a different build/architecture; its register labels and frame offsets were not treated as January x86 allocation evidence. RTC is not a complete scalar declaration-order or lexical-scope record.

## Moving: a rejected decompiler inference, not a new source proposal

One scratch moving shape was initially considered because Ghidra's decompiled C appeared to initialize `physics.velocity` from global zero and then assign `physics.new_velocity = physics.velocity`. Raw instruction verification disproved that direction before any admission.

RTC puts `physics` at `EBP-0xdc`. The current/source-compatible field offsets are input `velocity+0x2c -> EBP-0xb0`, and output `new_velocity+0xb8 -> EBP-0x24`. Actual /Od at `008c6ccc..008c6ce0` loads global zero into **new_velocity** at `-0x24/-0x20/-0x1c`; `008c6ce3..008c6cf8` then copies it into **velocity** at `-0xb0/-0xac/-0xa8`. Ghidra's high-level copy propagation had reversed the apparent source direction. January at `+0x0bb0..0x0bf5` uses one global-zero source for both vectors.

That actual direction is the previously tested UMG6 `physics.velocity = physics.new_velocity` hypothesis. It does not justify reopening the exhausted shape. The attempted opposite-direction scratch file `bipeds-moving-copy.c` measured 3,984 / 97, preserved 42 controls, and passed the emitted-point guard, but is **withdrawn because its claimed source evidence was wrong**. There is no admission JSON for it, and it must not be combined with the diagnostic candidate.

## Prior history respected and remaining boundary

Read the 100K wave-A/wave-D and 150K w2/w3 bipeds ledgers before shape work. They already cover moving's switch-field phi rewrites, player speed blends, opposite/new-velocity copy shapes, broad local-width/layout guesses, radius-argument parenthesizing, and related compiler switches. None of those is re-proposed here. The old missing-debug-header mechanism was explicitly reopened by root for a concrete authenticated source restoration rather than exact-match credit.

Physics remains constrained by the separately retained safe contact guard and unresolved scheduling/aggregate lifetimes. Moving's sphere-radius staging remains unauthenticated at the source-macro level. Raw RTC cannot resolve those scalar or inline-context questions by itself. This packet makes no additional shape proposal from an observed frame difference.

Evidence scripts: `inspect_bipeds.py` emits January/baseline assembly and unmodified aligned differences; `prepare_debug_block.py` reconstructs the concrete source/header review patch; `debug_global_provenance.py` preserves the unsuccessful data-name DIA queries; `review_final.py` audits the compiled final candidate. Whole-consumer/full-build checks and the decision to land the zero-credit diagnostic restoration remain root-owned.
