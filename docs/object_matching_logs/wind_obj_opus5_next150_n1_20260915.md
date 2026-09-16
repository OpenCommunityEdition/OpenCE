# `source/scenario/wind.obj` — next-150K lane, wave n1 (2026-09-15)

## Result

No source change. `source/scenario/wind.c` is byte-identical to the lane base `ae10935da`
(`git diff --stat -- source/scenario/wind.c` is empty).

| gate | exact | residual | unwritten | park drift |
| --- | ---: | ---: | ---: | --- |
| baseline (real file) | 7 | 2 | 0 | — |
| final (real file) | 7 | 2 | 0 | 0 of 2 |

The `_point_from_line3d` emitted-symbol guard passes. `tools/fake_match_scan.py` reports 0 leads.

One **park-reopen is proposed**: `_wind_variance_get` is strict EXACT at 224 bytes in the scratch candidate
`scratch/workers/n1_wind/v1.c`, taking the unit to 8 exact / 1 residual with zero sibling loss and zero drift of
the second park. This executes the assignment the 250K checkpoint (section 6.3) handed to the `w3d_wind`
worker, which was queued in `scratch/w3d/batch6-args.json` but never ran.

Working notes, with the full frame and relocation censuses:
`scratch/workers/n1_wind.md`. Candidates: `scratch/workers/n1_wind/{v1,v2,i1}.c`.

## `_wind_variance_get` (parked, `unclassified`) — REOPEN PROPOSED

- January: 224 padded / 213 meaningful, 8 relocations, sha16 `4df611f5e5a98c97`, frame `0x0c`.
- Real file (park baseline): 208 padded, 8 relocations, frame `0x0c`.
- Candidate `scratch/workers/n1_wind/v1.c` -> `v1.obj`: **224 / 8 / strict EXACT**.

### Census: there is no structural gap

The frames are identical (`sub esp,0xc`; `[ebp-0xc]/-8/-4` = the `axis_scale[3]` constants
`3dcccccd/3e4ccccd/3d8f5c29`; `[ebp+8]` and `[ebp+0xc]` the two stack parameters of the private register
convention, `eax` = wind, `edx` = position). The relocation multiset and every relocation address outside the
one differing block are identical. `alndiff` aligns 74 of 80 target instructions as equal.

The entire 16-byte difference is one construct:

```
JANUARY                                   OURS (HEAD)
63  fmul dword ptr [__real@41000000]      65  fmul dword ptr [__real@41000000]
69  fstp dword ptr [ebp + 0xc]            6b  fabs
6c  and  dword ptr [ebp + 0xc], 0x7fffffff
73  fld  dword ptr [ebp + 0xc]
76  xor  ecx, ecx                         (scheduled to 0x56)
78  fadd dword ptr [__real@4b000000]      6d  fadd dword ptr [__real@4b000000]
```

### The January bytes decide the question the checkpoint asked

The checkpoint required this to be decided from January's bytes: a sign-bit mask on the integer path, or
`fabs`. The bytes say **mask**, on four independent grounds.

1. January spills the float, applies a **32-bit integer `AND 0x7FFFFFFF` in memory**, and reloads it as a
   float. VC7 13.00.9254 canonicalises every floating spelling of absolute value to the x87 `fabs`
   instruction. That is proved by the current HEAD body, and `wind_obj_jonas_defined_fuzzy_closeout_20260831`
   additionally measured `fabs()`, a split assignment and the Marathon `ABS()` conditional — all `fabs`, all
   208 bytes. No floating spelling can produce a memory integer AND on the float's storage; that requires an
   integer lvalue aliasing the float.
2. The masked object is the same object that is then reloaded as a float, biased, stored back, and finally
   read one byte wide (`mov cl, byte ptr [ebp+0xc]`). One storage-shared object with three views — `real`,
   `long`, low `byte` — is exactly what the code does.
3. The 250K checkpoint's own byte-pattern census over every `build/split/**/*.obj` found `81 /4 FF FF FF 7F`
   at exactly one site in the whole January tree: this function. The idiom is deliberately translation-unit
   local, so no shared `cseries`/`real_math` macro is implied and **no header proposal is warranted**.
4. In-tree provenance for the construct already exists in production source:
   `source/ai/path_obstacle_avoidance.c:331-338` (`error_heap`) uses a block-scoped
   `union { real value; long bits; } cost;`, writing `.value` and reading `.bits`. `scratch/w2/laws_w2.md`
   section B lists "a local union bit transfer" under **Admitted, so don't avoid them**. HEAD's own body
   already reads this float's object representation through `byte const *`, so representation access here is
   settled policy; the candidate only routes the sign clear through the same representation.

### Minimum hunk

```c
		union
		{
			real value;
			long bits;
			byte bytes[4];
		} sample_key;
		long sample_index;

		sample_key.value =
			((axis_scale[axis_index] * wind_globals.time * local_variation_rate)
				+ position->n[axis_index]) * 8.f;
		sample_key.bits &= 0x7FFFFFFF;
		sample_key.value += 8388608.f;
		sample_index = sample_key.bytes[0];
```

That is the whole diff against HEAD. The history blob `5cfaf3cb5a08be127b927f2ef56e82337a2a0941` that the
checkpoint named also (b) makes `wind_variance_initialize` `static` (a linkage change needing symbols.json),
(c) deletes the `uniform_cubic_spline*` per-TU rename defines that keep January's out-of-line spline schedule
and the 10/10 text-owner census, and (d) retypes the control-point locals to `short`. All three touch
`_wind_variance_initialize`, would drift its park, and none is byte-proven. **None of them is carried.**

### Measured negative that fixes the union's shape

A two-member `union { real value; long bits; }` that keeps HEAD's `byte const *sample_key_bytes` low-byte read
(`scratch/workers/n1_wind/v2.c`) closes the size (224 / 8) but is **not** byte-exact. January reads the low
byte through the union's own byte view. The `byte bytes[4]` member is therefore evidence-backed, and it also
removes a pointer alias (laws_w2 R16).

### What the original programmer meant

Clear the IEEE sign bit to take the magnitude of the per-axis phase without a call; add the `2^23` bias so the
float's low mantissa byte is the integer part modulo 256; mask six bits to index the 64-entry per-axis variance
table. The three union members name exactly the three views the routine uses.

### Verification of the candidate

- whole-TU gate: `== exact 8  residual 1  unwritten 0`; every one of the 7 baseline-EXACT rows is still EXACT
  (row-by-row `--all` comparison).
- `_point_from_line3d` guard: passes.
- `scratch/parkcheck.py source/scenario/wind scratch/workers/n1_wind/v1.obj`:
  `OK _wind_variance_initialize 400 9` — the second park does not drift;
  `DRIFT _wind_variance_get 208 8 -> (224, 8, '4df611f5e5a98c97')` — that drift is the reopen itself.
- owner census vs `build/base` and `build/split`: candidate 23 owners, identical to HEAD base — 0 added,
  0 removed, 0 storage-class or section changes. The 8 `__real@*` COMDATs the candidate owns and January does
  not are the pre-existing accepted systemic select-any class (laws_w3 A30), unchanged from HEAD.
- `tools/fake_match_scan.py scratch/workers/n1_wind/v1.c`: 0 leads.
- `branch_sweep`: 9 unique blobs; only `5cfaf3cb`/`663bc667` reach 8 exact, and v1 reaches the same result
  without their three unrelated edits.

The park's recorded reopen criterion — "a defined, same-compiler full-width sign-clear idiom with credible
provenance" — is met, and its premise ("lawful VC7 `fabs` versus January full-width sign-bit clearing") is
refuted by the bytes: January never used `fabs` here.

## `_wind_variance_initialize` (parked, `tu-context-optimization`) — NOT REOPENED

- January: 384 padded / 380 meaningful, 9 relocations, sha16 `1d5cd879825797c6`, frame `0x38`.
- Real file: 400 padded, 9 relocations, frame `0x40`.

### Census

The relocation multiset is identical (9 / 9) and the seed phase is address-exact through `0x53`. The frame
holds the same 14 logical roles on both sides; January additionally reuses `[ebp-0x10]` (seed keyframe
countdown, then per-sample result pointer) and `[ebp-0x18]` (`cp-1` temp, then sample countdown). Ours needs
two extra dwords to home two extra pointer induction variables across the call.

### The blocker, named precisely

January's innermost (axis) loop recomputes three of the four keyframe addresses from ONE scaled integer
induction variable, `esi = axis_index * 8` (`add esi, 8`):

```
e2  lea eax, [esi + ecx]          ; ecx = the wrapped neighbour index, loaded from its stack slot
e5  lea edx, [eax + eax*2]
eb  shl edx, 5                    ; * 96
f0  add edx, &wind_globals.variance[0][0]
```

Only `f1` (`edi`) and the result (`ebx`) are pointer induction variables stepping `0x300`. VC7 on this source
strength-reduces **all five** addresses into five `0x300` pointer IVs and materialises `base + idx*96` once per
control point, which costs the two frame dwords and the 16 bytes.

That `(axis*8 + idx) * 96 + base` factoring is itself a byte proof that the retained source spelling is already
January's: `&wind_globals.variance[axis_index][neighbour_control_point * 8]` is the only form whose `64 * 12`
row stride and literal `* 8` share the factor 8 that VC7 hoisted into `esi`. The same non-reduced form appears
in `_wind_variance_get`, which is exact. So this residual is **not** a wrong type, a wrong local scope, a
missing statement or a wrong loop: it is VC7's induction-variable selection.

### Shape tried (1; wave w2 had already spent 5)

`scratch/workers/n1_wind/i1.c` deletes the three `word` locals and inlines
`(word)((control_point_index +/- k) & 7) * 8` in the call — wave w2's W1. It reproduces w2's measurement
exactly: 384 bytes, 9 relocations, `[sha]` only, frame `0x3c`. It still hoists all three neighbour addresses
(`0x8a`, `0xa8`, `0xba`), and it additionally loses January's control-point header, which the retained `word`
locals reproduce instruction for instruction (`mov cl,al; sub cl,2; xor edx,edx; mov dl,al; and edx,7;
and ecx,7; inc eax; mov [ebp-0x38],eax; and eax,7`). Size-closer but topology-farther; not retained.

No further shapes were spent: with the source expression byte-confirmed, anything else would be
induction-variable grinding.

### Refined reopen criterion

Reopen only with a measured VC7 rule that makes the compiler share ONE scaled integer induction variable
(`axis_index * 8`) across four `variance[axis][k * 8]` address expressions instead of creating a `0x300`
pointer IV per expression — an IV-count or register-pressure threshold demonstrated on a micro-lab, or
authentic January source. The park class should be restated from `tu-context-optimization` to
**backend induction-variable selection**. The retained `word` control-point locals and the
`variance[axis_index][x * 8]` index spelling are byte-confirmed correct and must not be changed.

## Do not repeat

- `fabs()`, a split assignment, or Marathon's `ABS()` for `_wind_variance_get`: all measured x87 `fabs`, all
  208 bytes.
- A two-member union plus a separate `byte const *` low-byte read: size-exact, byte-inexact.
- Porting blob `5cfaf3cb` wholesale: it bundles a linkage change, the spline rename removal and a control-point
  retype, none byte-proven, and drifts the second park.
- For `_wind_variance_initialize`: the explicit phase union, the one-local-per-target-slot spline graph, the
  result/current lockstep pointers, and w2's W1/W2/W4/W5/W6 index typings (W1 re-measured here).


## Orchestrator disposition 2026-09-15: HELD, not landed

The candidate is strict EXACT (224/8/`4df611f5e5a98c97`), the whole-TU gate goes 7 -> 8 exact with no sibling or park loss,
the owner census equals `build/base` exactly and the fake scan is clean. January's own bytes settle the construct:
`fmul [__real@41000000]; fstp [ebp+0xc]; and dword ptr [ebp+0xc],0x7fffffff; fld [ebp+0xc]; fadd [__real@4b000000]`.
A 32-bit integer AND applied in memory to a spilled float is not producible by any floating spelling, and HEAD's `fabs()`
body canonicalises to x87 `fabs` at 208 bytes.

It is HELD anyway because `wind_obj_jonas_defined_fuzzy_closeout_20260831.md` recorded 'it reads an inactive union member;
it remains rejected' for this function, and under laws_w3 R13 a recorded hold binds later waves and cannot be lifted by a
worker or a reviewer. Only the owner can lift it. The candidate is preserved at `scratch/opus5-next150/wind_HELD_candidate.c`.

EVIDENCE CORRECTION (verifier, byte-inert): this ledger's claim that the `byte bytes[4]` union member is evidence-backed
because v2 failed is a non-sequitur - v2 changed two things at once (dropped the byte member AND reintroduced a `byte const *`
alias). The verifier's control `scratch/verify/n1_wind/v3.c`, a two-member `union { real value; long bits; }` with
`sample_index = (byte)sample_key.bits;`, is ALSO strict EXACT. The byte member is a free stylistic choice; the two-member form
matches the three in-tree precedents and drops an endianness assumption. `bits & 0xFF` (v4) is NOT exact, so the truncation
must be to byte width.
