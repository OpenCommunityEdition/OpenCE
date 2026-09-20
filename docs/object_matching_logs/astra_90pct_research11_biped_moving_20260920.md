> Research-only packet11: zero production change and zero exact credit. Latest validated production remains wave9.

# Packet 11: biped moving — closed with zero shapes

No new admissible source mechanism was recovered for `source/units/bipeds.c::_biped_update_moving`. This packet performed **zero C compiles, zero candidate shapes and zero production changes**. Exact credit is zero. The January target remains 4,071 meaningful / 4,080 padded bytes; current production remains 4,000 padded bytes, with 97 relocations on each side and different normalized bytes. This is a research receipt, not an admission or a new compiler-gate result.

Production source is unchanged from `55843bda2c9ecb58fe5f1d52facb7e9f575eb1f2`. The read-only audit ran at metadata-only commit `030ec3a47bcd86ee44ba86b9dc729944b98a96f4`. `audit.json` binds current source, base object and January object hashes. Its direct COFF comparison finds all 42 inherited target-exact functions and the same nine residual target functions. The current object has 82 code owners, 193 named runtime owners, 111 nondebug noncode sections, four inherited COMMON variables and no `_point_from_line3d` definition. The complete runtime inventory is retained. No new before/after candidate comparison is claimed because no candidate was created.

## Fresh evidence and limits

`collect.py` directly reads the supplied Win32 image and reruns four DIA queries. `primary.json` retains image/PDB hashes, decoded instructions and all 12 RTC records. Region boundaries and every instruction start are checked against the existing actual Ghidra listing `scratch/astra-bipeds-evidence/ghidra_008c5840.asm`; this packet did not launch another Ghidra session. The correct entry is **0x008c5840**, not 0x008c7c50.

RTC records identify `physics` at EBP-220 with size 204, the three orientation vectors, the 244-byte player-information copy, the new position, movement vector, collision instance/results, point and plane. Fresh HCEA `biped_physics` type output corroborates the existing field order, `unsigned short` in/out flags, scalar floats and typed vector/point aggregates. It offers no missing narrow consumed-output declaration analogous to the separate weapons result.

HCEA identifies `static void biped_update_moving(long, struct unit_animation_update_data *)` at 0x017b2fc8 and source MD5 `026D0F6BE380914B377B1BF4A1CE0DAE`. HCEA and the later Win32 image corroborate particular operations/types. Their exact January source revision, original lexical text, complete scalar declarations and lexical scope remain unproven. RTC is an aggregate record, not a complete declaration-order record; decompiler stack-variable names cannot establish copy direction.

| Examined mechanism | Primary observation | Why it does not justify a new shape |
| --- | --- | --- |
| Actor-handle width | January +0x9c2 loads a full dword and +0x9cd pushes it to `_actor_is_leaping`. Later 0x8c693d/6943 does the same. Fresh DIA says the parameter is `long`. Current +0x99f instead sign-extends AX, reflecting the existing short cast. | This discrepancy is real, but old `workersD/bipeds/um1.c:3840` already removed the cast. Replaying that correction is not a distinct experiment. It is not inferred from weapons narrowing. |
| Suspended velocity outputs | Later 0x8c6cd4..6ce0 first initializes `new_velocity` from global zero; 0x8c6ce3..6cf8 copies it into input `velocity`. January +0xbb0..0xbf5 agrees on zero-valued outputs. | This is the previously tested UMG6 direction, also retained in old `um1.c:3940`. Packet 3's opposite-direction proposal came from misreading decompiled stack locals and remains withdrawn. |
| Animation fields, input flags and post-translation outputs | Raw switch code writes the physics movement fields; the flag graph operates on the 16-bit field; after translation, new velocity and output fields are consumed. | Direct field switch stores, player blend stores, direct `SET_FLAG(physics.in_flags, ...)`, biped flag captures, elevator ordering and output-flag variants were already explored. `history-witness.json` independently locates these key statements in old um1. No missing consumed store was recovered. |
| Radius argument | January +0xdc4/+0xdc7 stages the floating argument through x87; current +0xd79 uses an integer load/push. Later 0x8c7057/705d stages the typed bounding radius through SSE. | The later unoptimized argument staging does not prove the source wrapper or macro that would justify the old parenthesized-radius spelling. That spelling already has an explicit rejection history. |
| Boolean call predicates | Later 0x8c70f1 initializes a byte predicate, the collision result clears it at 0x8c7125, and it is tested at 0x8c712c..7135. Leap gating similarly materializes a boolean. | These are compatible with the current negated collision predicate and leap conjunction. The raw sequence does not distinguish an explicit source local from unoptimized lowering of the expression. Adding a guessed local would reopen the exhausted spelling family. |

The source's current short cast is therefore a known unresolved discrepancy, not a newly discovered exact-gain lever. No zero-credit fidelity edit was made. No physics diagnostic or out-of-bounds issue was included in this lane.

## Prior rejection boundary

Read before concluding: `bipeds_obj_opus5_100k_20260914.md`, `bipeds_obj_opus5_100k_waveD_20260914.md`, `bipeds_obj_opus5_150k_w2_20260914.md`, `bipeds_obj_opus5_150k_w3_20260914.md`, `astra_90pct_wave3_bipeds_packet_20260920.md`, and `astra_90pct_rejected_hypotheses_20260920.md`, all under `docs/object_matching_logs/`; also the original workers/bipeds.md, workersD/bipeds.md and workers/w3_bipeds.md notes in their archived worktrees. `history-witness.json` hashes the old um1 donor and preserves exact line witnesses.

Those records already cover switch phi/field variants, player/flying expression grouping, direct flags, velocity copy direction, output captures, scope/layout guesses, radius parentheses, helper/point ownership and compiler switches. Old n2 also had an undeclared `realcmp` dependency and is not an admissible donor. No old measurement is presented as a new compile here.

Reopen only for a distinct authenticated January source/local record, genuine owner or call boundary, consumed output, or natural compatible-compiler donor that explains a remaining machine difference without repeating these shapes. Later generic `/Od` temporaries, parentheses alone, a forced inline, tiny vector facade, local prototype or point-owner workaround do not meet that boundary. This packet establishes no general compiler law.

## Reproduction and disposition

From the worktree root, `python scratch/astra-wave11/biped-moving/collect.py` regenerates the bounded raw/DIA receipts using the supplied local artifacts. `python scratch/astra-wave11/biped-moving/audit.py` regenerates the current COFF census, exact controls, full January/current disassembly and alignment, and historical source witnesses. Both scripts are read-only with respect to production and invoke no C compiler or build.

`audit.json` is the structured result; `baseline.asm`, `target.asm` and `baseline.aln.txt` retain the current residual. The existing unit's owners/data/COMMON/point state is unchanged. There is no candidate, admission, whole-object gain or additional gate to publish. Evidence ownership is **RELEASED** to root; this lane is closed with zero credit.
