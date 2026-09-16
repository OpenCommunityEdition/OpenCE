# `s3tc.obj` relocation-census pass (opus5 next-150K lane, wave n4, 2026-09-15)

Unit `source/bitmaps/s3tc/s3tc.c`, baseline `ae10935da`. Assigned targets `_DecodeBlockRGB__single_pixel` and
`_DecodeBlockRGB`, both **parked**. All six non-exact rows in this object are parked, so **all work was scratch-only
and the real source file is unchanged** (`git diff --stat -- source/bitmaps/s3tc/s3tc.c` empty). No exact credit is
claimed by this wave.

Prior passes read in full: `s3tc_obj_opus5_next150_n3a_20260915.md`, `s3tc_obj_opus5_250k_w3c_20260915.md`,
`s3tc_obj_opus5_150k_w1_20260914.md`, `player_profile_s3tc_batch_reconciliation_20260907.md` (the batch that
installed the parks) and the four `s3tc_obj_jonas_*` ledgers. `git log --all` 9 commits; `branch_sweep` best
historical blob `dc1eb5bd` (same 12/6/0, sizedelta 464) — already read and evaluated by n3a.

## Baseline and final gate

| gate | result |
| --- | --- |
| baseline (`--all`, `_point_from_line3d` guard) | exact 12 / residual 6 / unwritten 0; guard passed |
| final real file | identical: exact 12 / residual 6 / unwritten 0; guard passed |
| park drift (`scratch/parkcheck.py`, real object) | 6 parks, drift 0 |
| fake-match scan (real file and candidate `d2.c`) | 0 review leads each |
| real file modified | **no** |

## 1. What the census actually says

The wave assignment was a relocation-by-target census delta on both targets: *ours owns `{_RGBToColor: 1}` that
January does not*. Computed here against the real HEAD object
(`relocdiff --count-by-target --allow-structural`):

| function | target | JAN | OURS |
| --- | --- | --- | --- |
| `_DecodeBlockRGB` | `_csmemset` | 1 | 1 |
| | `_RGBToColor` | **1** | **2** |
| | TOTAL | 2 | 3 |
| `_DecodeBlockRGB__single_pixel` | `_csmemset` / `_display_assert` / `_system_exit` / file-name literal | 1 / 2 / 2 / 2 | 1 / 2 / 2 / 2 |
| | `_RGBToColor` | **1** | **2** |
| | `??_C@_0N@CLDKFKKD@` = `u>=0 && u<=4` | **1** | **0** |
| | `??_C@_0N@NBFAEMLN@` = `v>=0 && v<=4` | **1** | **0** |
| | `??_C@_0BA@MDFLADHG@` = `u >= 0 && u < 4` | **0** | **1** |
| | `??_C@_0BA@GMAHMFCK@` = `v >= 0 && v < 4` | **0** | **1** |
| | TOTAL | 10 | 11 |

**Frame census: `sub esp,0x10` on both sides of both functions**, every `[ebp±N]` slot agreeing — no missing and no
extra local. The entire 64-byte gap in each function is code, not frame.

The extra call is the **"January inlines it"** case, visible directly in January's bytes. In `_DecodeBlockRGB`:

```
   7: mov edi, dword ptr [ebp + 8]     ; the `source` parameter is consumed here; the slot is now dead
  29: mov dword ptr [ebp + 8], esi     ; VC7 re-homes the INLINED RGBToColor there (esi = zero-extended *edi)
  2c..64                               ; the inlined helper body, in the dead parameter slot
  70: mov dword ptr [ebp - 0x10], edx  ; ... stored into color[0]
  73: call _RGBToColor                 ; the SECOND call, out of line
```

Because the inlined copy is homed in the caller's own dead parameter slot it costs **zero** frame bytes — which is
exactly why the frames agree while the code sizes differ by the size of the helper body.

## 2. The cause is one instruction in the callee, measured in both directions

The boundary is owned by `_RGBToColor`, not by either caller. Single-variable A/B at this HEAD:

| candidate | `_RGBToColor` | `_DecodeBlockRGB` | `_DecodeBlockRGB__single_pixel` | whole-TU gate |
| --- | --- | --- | --- | --- |
| real file (HEAD) | 96 / 0 `[sha]` | 384 / 3 | 432 / 11 | exact 12 / residual 6 |
| `scratch/workers/s3tc/reopen_rgbtocolor.c` (Microsoft/S3 reference order, alpha member left unwritten) | **EXACT 96 / 0** | **EXACT 448 / 2**, census 2/2 **0 differing rows** | 496 / 10 `[sha]`, census 10/10, 2 differing rows | **exact 14 / residual 4** |
| `scratch/workers/n4_s3tc/s1_alpha_last.c` (= reference + `color.rgba[S3TC_ALPHA] = 0;` last) | 96 / 0 `[sha]` | 384 / 3 | 432 / 11 | exact 12 / residual 6 |
| `scratch/workers/n4_s3tc/s2_alpha_first.c` (= reference + the same store first) | 96 / 0 `[sha]` | 384 / 3 | 432 / 11 | exact 12 / residual 6 |
| `scratch/workers/n3_s3tc/d2.c` (reference + January assert text + 64-byte clear) | **EXACT** | **EXACT** | **strict EXACT 496 / 10**, census **0 differing rows** | **exact 15 / residual 3** |

`alndiff` on `s1_alpha_last.obj` gives the mechanism to the byte: the explicit alpha store does **not** destroy the
`push ecx` slot merge inside `_RGBToColor` (this corrects w1's "any initialiser destroys the merge", which was
measured on `= {0}`, not on a plain store); it adds **exactly one instruction / 4 bytes**,
`mov byte ptr [ebp-1], 0` at `O 0x47`. That one instruction pushes the helper past VC7 13.00.9254's auto-inline cost
threshold, so neither call site is inlined in either caller — **+1 relocation and −64 bytes in each of the two
targets, from one instruction in the callee.**

**Park premise refuted, verbatim.** `_DecodeBlockRGB`'s park evidence reads "remaining instruction selection and
scheduling are non-exact". A relocation-count delta is neither: the two objects disagree on how many calls exist.
`_DecodeBlockRGB__single_pixel`'s park text lists the clear and the assert predicate but **does not mention the extra
`_RGBToColor` call at all**, so this defect has never been recorded against that row.

## 3. New January-byte evidence on the blocking clause

The blocker is the 2026-09-07 park clause on `_RGBToColor`: *"do not restore the original indeterminate alpha read to
regain bytes."* Three facts recovered this wave that were not in w1 or n3a:

1. **There is no uninitialised read in January's machine code.** The out-of-line body materialises `rgb`
   zero-extended into the merged slot and writes the *whole dword*:

   ```
    3: push ecx                     ; the single 4-byte frame slot [ebp-4] = rgb and color merged
    4: xor ecx, ecx
    6: mov cx, word ptr [eax]
   16: mov dword ptr [ebp - 4], ecx ; byte [ebp-1] (the alpha) is written 0 HERE
   27/37/41: byte stores of red / green / blue
   44: mov edx, dword ptr [ebp - 4]
   4a: mov dword ptr [eax], edx     ; *pcolor = color, one dword
   ```

   The same holds at the inlined site (`T 0x24 xor esi,esi; T 0x26 mov si,word ptr [edi]; T 0x29 mov [ebp+8],esi`).
2. **The byte is dead at every call site in the translation unit.** `s3tc.c:592` and `s3tc.c:653` both execute
   `color[0].rgba[S3TC_ALPHA] = color[1].rgba[S3TC_ALPHA] = color[2].rgba[S3TC_ALPHA] = 0xFF;` immediately after the
   two calls — January emits it four instructions after the inlined copy
   (`T 0x7e/0x82/0x86 mov byte ptr [ebp-5]/[ebp-9]/[ebp-0xd], 0xff`) — and `_Quantize` (`s3tc.c:208-210`) passes the
   colour only to `ColorToFcolor`, whose body (`s3tc.c:113-121`) reads `rgba[0..2]` only.
3. **Language classification (argument for the owner, not a byte proof).** C11 6.3.2.1p2 makes an indeterminate read
   undefined only for an automatic object that never had its address taken **and** to which no assignment has been
   performed. `color` has `rgba[0..2]` assigned before `*pcolor = color`, and `byte`/`unsigned char` has no trap
   representation (6.2.6.1p5). The read is an **unspecified value, not undefined behaviour** — a weaker category than
   the worker brief's prohibited "uninitialised reads".

**No non-UB spelling exists.** Any explicit write to the fourth member is one instruction, and one instruction is
exactly what flips the inliner (shapes `s1`/`s2` above). The three rows stand or fall on this single ruling.

## 4. The other four residuals — census is clean

| function | census | outcome |
| --- | --- | --- |
| `_RGBToColor` | 0 / 0, no delta (leaf) | coupled to the ruling above |
| `_DecodeBlockAlpha3__single_pixel` | **1 / 1, 0 differing rows** | SKIPPED-EXHAUSTED — no call leverage; n3a accounted the 11-byte gap to four integer promotions and classified the remainder as the laws_w3 A43 cross-jump register web after 8 failed shapes |
| `_Quantize` | TOTAL 30 / 30 | SKIPPED-EXHAUSTED — the 9 "differing" rows are 6 copies of one constant `__real@437f0000` reported as `defined-noncode:.rdata` on January's side and `symbol` on ours: a **naming artifact, not a missing call** |
| `_EncodeBlockRGBColorKey` | TOTAL 36 / 36 | SKIPPED-EXHAUSTED / OWNER-BLOCKED — **all 36** rows are the same artifact over `__real@00000000` (9/9), `__real@3f800000` (2/2), `__real@40000000` (1/1), `__real@40400000` (4/4), `__real@40800000` (2/2); the 448-byte gap has no relocation leverage and is the already-solved unroll structure gated on three original forms |

0 shapes were spent on these four.

## 5. Rejected / not proposed

- Landing anything in the real file: all six rows are parked.
- Any `_RGBToColor` form that writes the fourth member (`s1_alpha_last`, `s2_alpha_first`, and w1's `= {0}`): measured
  to revert all three rows.
- Steering the inliner from inside either caller: both caller bodies are already byte-exact under the reference
  helper, so any change there would be pure inliner-budget steering with no source evidence.
- Re-running the park-premise sweep n3a exhausted, and any register/x87 work (owner directive for this wave).

## 6. Orchestrator proposals

1. **Park reopen, three rows, one commit, 1,040 padded bytes** (96 + 448 + 496) — candidate
   `scratch/workers/n3_s3tc/d2.c` (whole-TU gate **exact 15 / residual 3**), with
   `scratch/workers/s3tc/reopen_rgbtocolor.c` (**exact 14 / residual 4**) isolating the two rows that need only
   ruling (a). Rulings required: **(a)** `_RGBToColor`'s unwritten fourth member (section 3); **(b)** restore
   January's assert text `u>=0 && u<=4` / `v>=0 && v<=4` per `docs/assertions.md` — not UB, closes 4 of the 5
   remaining instructions; **(c)** the 64-byte clear under the original-bug policy with a
   `/* BUG (preserved for exact matching): */` comment and the same-TU precedent at `s3tc.c:586`. The commit must
   also remeasure the `_Quantize` park to 528 / 30 / `57685b731729ce83`.
2. **`config/parked.json` evidence-text corrections (text only, zero bytes).**
   - `_DecodeBlockRGB`: replace "remaining instruction selection and scheduling are non-exact" with the measured
     cause — one extra out-of-line `_RGBToColor` call (census 2 vs 3); January inlines the first call and the
     boundary is set by the callee body, not by this function.
   - `_DecodeBlockRGB__single_pixel`: add the third, unrecorded defect (the same extra `_RGBToColor` call) and
     replace "Require coordinates 0..3, not the original inclusive 4" with January's byte-proven text.
3. **Census-tooling caveat, validated here.** `relocdiff --count-by-target` reports the *same* `.rdata` literal as
   `defined-noncode` on the January side and `symbol` on ours. On `_EncodeBlockRGBColorKey` that produced **36 of 36
   "differing" rows with a perfectly equal multiset**. A census delta is real only when the *decoded name* differs;
   a lane-wide census sweep must compare decoded names or it will manufacture leverage that does not exist.

## 7. Reopen criteria

- `_RGBToColor` / `_DecodeBlockRGB` / `_DecodeBlockRGB__single_pixel`: owner acceptance of a written-3-of-4 aggregate
  copy whose fourth member is dead at every call site in the TU and provably zero in January's emitted code, plus
  (for the single-pixel row) the assert-text restoration and the original-bug clear.
- `_Quantize`: unchanged — authentic S3/Microsoft encoder source, or a measured x87/integer interleave law.
- `_EncodeBlockRGBColorKey`: unchanged — rulings on the three original forms.
- `_DecodeBlockAlpha3__single_pixel`: unchanged — a natural source distinction for the EDX accumulator and reload
  order; n3a additionally measured the Microsoft 2003 corpus to contain no single-pixel helper.
