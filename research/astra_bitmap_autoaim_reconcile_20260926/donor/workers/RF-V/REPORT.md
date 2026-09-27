# RF-V wave-5 report (saved by the lead)

Lane HEAD 3a319d61; copies of HEAD bipeds.c and motion_sensor.c rebuild build/base exactly (keyed 0/0/0; gates bipeds
43/8, motion_sensor 17/2). Patch-only; no lane source/config/tools/build edits; no ninja; no commits. Records:
NOTES.md, cards V00-V17, evidence/, patches, CANDIDATE_biped_get_autoaim_pill.txt; scratch `scratch/rf/workers/RF-V/`.
Every card written with a clock-read timestamp before its compile or trace; every traced object equals the stock
compile.

## 1. `_biped_get_autoaim_pill` (source/units/bipeds): EXACT candidate (6 card-driven compiles)
Step 1 readout (V01-V03): global allocator (costs.py): all 12 global webs get January's registers; the spherical
tail's pointer and value are not global webs. IL at MARK (ilcap): one tail block after the outer if/else (a value load
then a store through `[width]`); the spherical arm reaches it by a jump, the non-spherical arm by a jump, the physics arm
falls through. Step-pass snapshot (stepwalk): by step #1 (post-allocation) the backend return split has already made a
separate tail copy in each arm, each coloured on its own (spherical value ECX / pointer EDX; non-spherical value EAX /
pointer ECX; physics value ECX / pointer EDX); the scheduler then hoists the EDX pointer load into the
`*height = *global_zero_vector3d` copy. January's spherical tail has the non-spherical colours. Deciding fact: whether
the width store is part of the return block (HEAD: copied into each arm before allocation) or of an ordinary join block
(allocated once, cloned after allocation). Prediction: make the inner if/else end in a shared store block.
Step 2 evidence: /Od `halo_cache_symbols.exe` 0x8bffb0 has two separate `*width = definition->biped.autoaim_width`
stores: +0x10c..+0x118 at the end of the outer if-arm after the inner join (the spherical arm's `jmp` lands on it; the
non-spherical arm falls into it) and +0x176..+0x182 at the end of the physics else-arm. Not a recorded negative (Lane B's
aa1 put one store in each of the three innermost arms; bytes = base).
Candidate V04 (`biped_get_autoaim_pill_V04_exact.patch`): moves the single trailing store into those two places; nothing
else.
| Check | Result |
|---|---|
| gate --all | 43/8 -> 44/7; only this row changes (residual [sha] -> EXACT 336) |
| keyed diff vs build/base | 195 -> 195: 1 changed (EXACT vs January), 0 added, 0 removed, 0 losses |
| /W3 /Zs | 15 = 15 (same set apart from line numbers; none in this function) |
| fake_match_scan | 0 (HEAD 0) |
| patch | `git apply --check` clean; replaying on the pristine HEAD blob reproduces the candidate byte for byte |
| strip test | one indivisible attested element; removing it gives HEAD (residual); aa1 shows stores per innermost arm do nothing, so the load-bearing part is the shared store after the inner if/else |
Mechanism confirmed (V05, stepwalk of V04): at step #1 the spherical arm ends in a jump to the join block coloured
EAX/ECX; the final spherical tail carries those colours (clone made after allocation). Corroboration (V17): January's
pill equals the Aug-15-2001 and Sept-25-2001 debug builds (0 hunks each), and V04 equals Sept-25 too. Alternative (V06):
V04 plus the /Od condition without the `pelvis_node_index` local gives a byte-identical whole object. Disclosure (rules
22, 31): the store now appears once in each outer arm (two arms end in equivalent statements); /Od attests this layout
and never merges statements; no decoration, local, cast, header or symbols.json change. parked.json not edited by the
worker. Failed prediction: V01 predicted the spherical-tail colours came from the global chooser; they are block-local.

## 2. `_render_motion_sensor` (source/interface/motion_sensor): improved, not exact (~11 of 30 compiles/traces)
Frame readout (V10, Brief G predictor): reproduces every item of our frame (compare.py all 1s); the six FRAME events come
from the two-variable form (the inner trip-counter temp shares weight's [ebp-8]; radius gets its own [ebp-0x20]).
V11 (`render_motion_sensor_V11_improved_not_exact.patch`): one `weight` local reused
(`weight = (real)(pow(1.0f - weight, 3.5) * 7.0 + 1.0);`: /Od stores the pow result back into the weight slot at
0x642868 and pushes it as the radius; January's `fstp [ebp-8]` agrees) and direct `pow` instead of `power()` (/Od calls
`pow<float,double>`). Gate 17/2 (still residual 784 vs 768); keyed diff 1 changed (residual), 0 added, 1 removed (the
surplus `_power` COMDAT, absent from January's object); FRAME swap gone; /W3 17 = 17; fake scan 0. Strip (V15): the
one-variable form alone buys the frame fix; direct pow alone only removes `_power`. Zero credit; the park would need
re-baselining.
Remaining residual (V12-V14, V17): the loop-head block (+2 bytes), every later pad driven by that offset. (a) The fold:
at MARK, SUB takes history_index as a symbol operand; our `mov edx,[ebp-0x18]` is a reload created after MARK; January
keeps the memory operand; a watch on the SUB node's operand-list heads never fired (V16), so the deciding site was not
found. (b) The order: scheduler trace (rfsched) with our dependence graph dual-issues the x87 chain as soon as ready
(fmul cycle 78, before idiv at 83); the player reload waits for idiv until cycle 105; no key or list order gives
January's order under this model; January needs `fmul 0.1f` to wait for idiv and the player reload. The Aug-15-2001
debug build (earlier revision) shows January's x87-after-idiv order without the fold (reloads into ESI), so (a) and (b)
are independent; (b) is shared by two source revisions and missing from ours; /Od's loop-head statements match ours, as
does the Sept-25-2001 debug build. Failed predictions: V13 (EDX load expected to be a coloured global web; it is not);
V16 (operand-list heads expected rewritten; they were not).
Reopen: first-party evidence of a construct common to the Aug-15-2001 and January loop heads that makes the weight
chain's fmul depend on the sensor-index idiv or the player reload (check with rfsched: fmul must not issue before idiv);
the January-only fold is a separate post-MARK reload decision. Do not respend statement order, division form, index
spelling, declaration order, name/local counts or the stack sentinel (all recorded inert).

## Lead verification (2026-09-26)
/Od 0x8bffb0 (scratch/rf/lead/od_8bffb0.txt): two `*width = definition->...` stores at +0x10c..+0x118 (then `jmp` to the
exit) and +0x176..+0x182 (after the scale_vector3d call): confirmed. V04 applied in the lane: gate 44/7,
`EXACT 336 _biped_get_autoaim_pill`; keyed diff vs build/base 195 -> 195, 1 changed (EXACT vs January), 0 added,
0 removed; diff --check clean. Park entry retired (its text: "Reopen only with authoritative January source/local
records or a natural compatible-compiler donor explaining the register schedule"); 72 entries remain. Full clean-build
checkpoint R4 pending.
