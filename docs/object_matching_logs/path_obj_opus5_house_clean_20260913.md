# path.obj: opus5 house-clean lane (2026-09-13)

## 1. Scope and provenance

- Translation unit: `source/ai/path.c`; target object `build/split/source/ai/path.obj`.
- Compiler: VC7 13.00.9254 (XDK), default unit flags `/nologo /c /O2 /Oy-` (no per-unit override).
- Lane: `opus/50k-house-clean-20260913`, pinned base `73e97eca51425495acb9dbfa3349370c7962c022`.
- Work pool: `_path_state_build_path` (residual) and `_code_0004c7b0` (unwritten).
  The eight `config/parked.json` path functions were locked and not edited.
- Evidence: January split object (instruction stream, relocations), `cachebeta.exe`
  read as data for caller/reference scanning, HCEA `path_state_build_path.c` and the
  HCEA function address list (semantic/naming corroboration only).

## 2. Validated baseline

Fresh real-file gate before any edit:
`== exact 22  residual 9  unwritten 1  (of 32 listed)`; fake-match scan 0 leads.

| Function | Target size / relocs / sha | Final candidate size / relocs / sha |
| --- | --- | --- |
| `_path_state_build_path` | 1184 / 37 / `e43c741f08a41afd` | 1184 / 37 / `e43c741f08a41afd` (EXACT) |
| `_build_path_edges_for_surface` | 384 / 13 / `3f8e92ea9ce08fb3` | 384 / 13 / `d0a1a65d568c291b` (parked) |
| `_closest_point_to_attractor` | 208 / 2 / `5d7c8671785a0916` | 208 / 2 / `d96eb8493bcb04e1` (parked) |
| `_path_attractor_weight` | 192 / 6 / `9785d44ef2192e89` | 192 / 6 / `ff4988ccef4e2ad2` (parked) |
| `_path_heap_bubble_down` | 560 / 24 / `7a171cedd6dae59c` | 544 / 24 / `633165e5c90816f6` (parked) |
| `_path_state_approach_point` | 304 / 13 / `d5abe183e0dbbae5` | 304 / 13 / `9f758f9afb195dbb` (parked) |
| `_path_state_begin` | 592 / 20 / `039ed79debe1e371` | 608 / 20 / `ad863f4591f7a569` (parked) |
| `_path_state_estimated_distance` | 496 / 13 / `24719f50ab8c668f` | 528 / 13 / `e286347823095bc8` (parked) |
| `_path_state_traverse` | 2096 / 65 / `3e33ddd8b81a7b51` | 2096 / 65 / `f52af9dafed6defa` (parked) |
| `_code_0004c7b0` | 16 / 0 / `1191f37eec740e49` | absent (owner-blocked name) |

Baseline first divergence of `_path_state_build_path`: January 0x2e2, immediately after
`call _path_avoid_obstacles`; every other instruction was already aligned.

## 3. Accepted control (retained in production)

`_path_state_build_path` is now strict exact (**+1 function, 1184 meaningful / 1184 padded
bytes, 37 relocations**). The only change moves the obstacle-avoidance failure record
into the arm that actually runs avoidance:

```c
path_build_success = path_avoid_obstacles(...);

if (state->debug && !path_build_success)
{
	state->debug->path_build_result =
		_path_build_result_obstacle_avoidance_failed;
}
```

and removes the trailing `else if (state->debug)` arm after the success block.
Behaviour is unchanged: the disabled-avoidance arm always succeeds, so failure can only be
recorded where `path_avoid_obstacles` returned FALSE.

Binary evidence (January 0x2dd-0x303):
`call _path_avoid_obstacles; mov ecx,[ebx+0x48]; add esp,0x18; test ecx,ecx; je A;
test al,al; jne S; mov word [ecx+0x12],4; jmp end; A: test al,al; je end; S: ...`.
- `state->debug` is loaded in the call's own block (before the stack adjust), so the debug
  test belongs to that arm.
- The duplicated `test al,al` proves a debug-first `&&` test followed by a separate
  success test (`if (path_build_success)`), jump-threaded.
- The disabled-avoidance arm (0x2c3) jumps straight into S, which only happens when the
  failure test is not on its path (see shapes 1/2 below, which materialise `mov al,1`).
- The debug-first `state->debug && ...` guard is the same idiom January uses in
  `path_state_traverse`.

## 4. Experiment matrix

| # | Shape | Result | Siblings | Kept |
| --- | --- | --- | --- | --- |
| 0 | baseline: `if (success) {S} else if (state->debug) {failed}` | 1184/37 sha differs, [sha] | 22 exact | - |
| 1 | `if (state->debug && !success) {failed}` after the if/else, then separate `if (success) {S}` | 1200/37 [size]; disabled arm emits `mov al,1` + extra test; `add esp` ordered before debug load | 22 | reverted |
| 2 | as 1 but `else if (success) {S}` | 1200/37, identical stream to 1 | 22 | reverted |
| 3 | failure record inside the avoidance else-arm (above) | **1184/37 strict EXACT** | 23 (no EXACT row lost) | landed |

Checks for shape 3 (candidate and final real file): `--forbid-emitted-symbol
_point_from_line3d` passed; parked drift 8/8 records identical to `measurements.base`;
owner census (code names and data section owners) identical to the base candidate
(extra helper COMDATs `_distance3d` etc. and target-only `_code_0004c7b0` unchanged);
`tools/fake_match_scan.py` 0 leads (baseline 0).

## 5. Do-not-repeat

- Do not reopen `_path_state_build_path`; it is exact.
- Shapes 1/2 (failure test at the merge after the if/else) are measured and wrong.
- Do not write `_code_0004c7b0` under a `code_` name or invent a name in the TU.

## 6. Residual classification

- `_code_0004c7b0` - naming/owner. Measured fact: bytes `b0 01 c3` (`mov al,1; ret`), a
  boolean function returning TRUE that touches no arguments. Link order places its
  definition between static `path_state_reset` and static `path_heap_bubble_up`. A
  data-only scan of `cachebeta.exe` finds zero rel32 call/jmp sites to VA 0x44C7B0 and zero
  absolute references, so it is an unreferenced retained external function (an uncalled
  static would not be emitted). Inference: a stub/validation routine whose body was
  compiled down to `return TRUE;`. HCEA (Xbox release) has nothing between
  `path_state_reset` @0x837D2518 and `path_heap_bubble_up` @0x837D2588, and hs7 cross-build
  names have no match, so no authentic name is available.
- The eight parked functions are unchanged and were not examined for new shapes.

## 7. Reopen criteria

- `_code_0004c7b0`: an owner-supplied authentic name (symbols.json rename from PDB or
  cross-build symbol evidence for a TRUE-returning path.c function in that link slot) plus a
  genuine `path.h` prototype.

## 8. Disposition

Object gate after this lane: **23 exact / 8 residual / 1 unwritten (of 32)**, up from
22/9/1. The object is still active (NonMatching); eight residuals are parked and one leaf is
owner-blocked on naming. No header, config, or symbols change was made.
