# RF-A report: `_actor_path_refresh` (source/ai/actor_moving.c): EXACT candidate (saved by the lead)

**Verdict: EXACT.** One change taken from the /Od build makes the function byte-identical to January: gate
`EXACT 1440 _actor_path_refresh`, 52 relocations, normalized sha16 b6b01d01147885d0 = January. Unit 32/4 -> 33/3,
0 losses. 4 of 30 card-driven compiles used; the lane's source was not edited by the worker; nothing committed.

## The change (P02, recommended; `actor_path_refresh_P02.patch`, sha256 1524e285...)
The /Od build `halo_cache_symbols.exe` at 0x46c720 (offsets 0x46cbc1-0x46cc2b) stores a flag TRUE before
`actor_test_destination` is called; if the call returns FALSE it jumps straight to the flag test; if TRUE the flag
takes `have_previous_destination && distance_squared3d(...) > 0x3c23d70b` (0.1f*0.1f folded) through the usual `&&`
temporary; then `if (flag)` guards the rebuild. Our `!a || (b && c)` condition cannot store TRUE before the call, so it
is not the same source. P02:
```c
boolean build_path = TRUE;

if (actor_test_destination(actor_index))
{
	build_path = have_previous_destination &&
		distance_squared3d(
			&previous_destination,
			&actor->control.path.destination.point) > 0.1f*0.1f;
}

if (build_path)
```
Alternative with identical bytes: P03 adds the /Od's `if (!path_available) { success = FALSE; } else { ... }`
nesting, which has no effect on January's bytes (card P01): January neither supports nor contradicts it; choosing
P02 vs P03 is an owner decision. The name `build_path` is descriptive (the /Od build names aggregates only); style
precedent `boolean build_attack_vectors` (actor_firing_position.c:1971); disclosed under house rule 15.

## Why the three jumps bind where they do
Bytes: the three failure edges of the late `if (success && endpoint.target_radius > 0.f && ...)` guard (January
0x50e `jne`, 0x51e `jp`, 0x534 `jp`) bind short to the epilogue at 0x542 (after the late `actor_path_clear`) in
January; in ours they bind near to the epilogue at 0x2dd (after the `if (!success) actor_path_clear` block behind
`default:` at 0x2d1 in both builds). Instructions otherwise identical (421 = 421, 0 masked differences); the 3 x 4-byte
jump widths are the whole residual. /FAsc: lane source's canonical exit `$L14919` follows the `!success` clear, and the
constant-TRUE exit (`mov al,1`, `$L14916`) has a lower label number than the variable-value exit; with the patch the
canonical exit is `$L14922` at 0x542 (source-last position, the usual case, 514 of 563 groups per F5), the `!success`
clear gets an unlabelled clone, and the constant-TRUE exit `$L14923` numbers after it. Inference (not traced in C2):
the flag form changes the order in which the two value classes of `return success` are created, consistent with
LAW B (returns merge on value; the canonical block owns the `$L`).

## Cards (written before each compile)
| Card | Shape | Result | Prediction |
|---|---|---|---|
| P01 | /Od nesting only (no flag) | no effect (sha16 89b79760) | no effect: held |
| P02 | flag only | EXACT | no effect (~80%): WRONG |
| P03 | nesting + flag | EXACT, same bytes as P02 | "flips only if nesting flips": WRONG |
| N1 | control (not landable): an unused `build_path` at P02's position | no effect | no effect: held |
N1 shows the flip comes from the flag's structure, not from one more declared name.

## Landing checks (worker)
Keyed diff vs build/base actor_moving.obj: 1 changed section, 0 added, 0 removed, 0 losses; `_actor_move_update`,
`_actor_move_vector_avoidance`, `_actor_move_test_avoidance_vector` byte-unchanged; no held form touched. Raw sections:
`.debug$S` differs only by the compiled file path; two functions differ only in local `$L` jump-table label numbers;
415 symbols before and after; no new COMDAT, data or public symbol. /W3 /Zs: 12 warnings before and after, all in
cseries.h / real_math.h, none in actor_moving.c. fake_match_scan: 0 leads. Strip test: P02's single change accounts
for all 12 bytes; in P03 the nesting accounts for 0 and the flag for 12.

## Records contradicted
Lane A's A43 ("no source lever moves the survivor"), next150 section 4.1's "no admissible source reaches strict
exact" for this function, and W3 section 0 are wrong for this function; none of the 16+ recorded spellings, v1-v6 or
pr1 tried this flag shape. The /Od build also initialises `avoidance_distance = 0.f`, which January's bytes refute
(no store before the call that receives its address); not used.

Files: NOTES.md, cards/, actor_path_refresh_P02.patch, actor_path_refresh_P03.patch, keyed_diff.txt, secraw.txt,
evidence/ (gate outputs, measurements, /FAsc excerpts, /W3 output, tools); scratch under scratch\rf\workers\RF-A\.

## Lead verification (2026-09-26)
Independent /Od readout at 0x46cbb0: `mov byte [ebp-0x4d],1` before `call 0x401ab4` (actor_test_destination),
`je 0x46cc25` to the flag test, the `&&` temporary at [ebp-0x1414d] stored into [ebp-0x4d], then
`movzx eax,[ebp-0x4d]; test; je`: a real local with its own slot, matching P02. P02 applied in the lane: gate 33/3,
`EXACT 1440 _actor_path_refresh`; keyed diff vs build/base: 1 changed (EXACT vs January), 0 added, 0 removed; fake
scan 0; diff --check clean. Full clean-build checkpoint pending (the shared build is in use by the other workers).
