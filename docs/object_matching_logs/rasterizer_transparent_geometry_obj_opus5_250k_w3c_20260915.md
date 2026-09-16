# `rasterizer_transparent_geometry.obj` — Opus5 250K lane, wave w3c (2026-09-15)

Lane `opus/250k-house-clean-20260915`, worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
Scope: `source/rasterizer/rasterizer_transparent_geometry.c` only. First w2/w3-law pass over this object.

## Result

**No source change landed.** `git diff --stat -- source/rasterizer/rasterizer_transparent_geometry.c` is empty.

| gate (real file, `--all --forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 17 | 2 | 0 |
| final | 17 | 2 | 0 |

Guard: pass. `scratch/parkcheck.py`: 2 parks, drift 0. `tools/fake_match_scan.py`: 0 review leads. Census: unchanged.

The wave's substantive result is that `_rasterizer_transparent_geometry_initialize`'s residual has been reduced, in scratch,
from an instruction-selection difference to a **pure basic-block-placement** difference, and the surrounding source-shape
space has been mapped with six measured shapes.

## `_rasterizer_transparent_geometry_initialize` (160 B, parked) — not landed; residual narrowed to block order

Target `63251d0d8cc730b0…` / 16 relocs; park base `15f6dc8cbb204d55…` / 16; objdiff 76.0.

January's tail, in layout order:

```
        test eax,eax                ; transparent_geometry_groups2
+0x75   jne  +0x89                  ; -> aux block
+0x77   push <"### ERROR failed to allocate transparent geometry buffer">
        push 2 ; call _error ; add esp,8
+0x86   xor  al,al
+0x88   ret
+0x89   call _rasterizer_transparent_geometry_initialize_aux_buffer
+0x8e   test al,al
+0x90   je   +0x86                  ; shared "return FALSE"
+0x92   mov  al,1
+0x94   ret
```

The park base instead emits `call aux; test al,al; setne al; ret` and sinks the error block to the end of the function.

### Shapes measured (six; all scratch, `scratch/workers/w3c_rasterizer_lights_shader_transparent_chicago_preprocessor_etc/tg/`)

| id | source shape | result |
| --- | --- | --- |
| t1 | `boolean result = TRUE;` + `if (!A\|\|!B\|\|!C) { error; result=FALSE; } else if (!aux()) { result=FALSE; } return result;` | 160/16; recovers `mov al,1`, but the aux block is still first and a bare `ret` replaces the shared false exit |
| t2 | `boolean result = FALSE;` + `if (…) { error; } else if (aux()) { result=TRUE; } return result;` | **144 bytes** — the FALSE initializer pins the constant 0 in EBX and respells the null tests as `cmp [global], ebx`; refuted |
| t3 | `if (A&&B&&C) { if (!aux()) result=FALSE; } else { error; result=FALSE; }` with `result = TRUE` | byte-identical to t1 — VC7 canonicalises `&&` against `\|\|` polarity |
| t4 | t3 with early `return FALSE` in both arms | byte-identical to the park base (SETNE form) |
| t5 | `if (!A\|\|!B\|\|!C) { error; } else if (aux()) { return TRUE; } return FALSE;` | **instruction-identical to January**: identical multiset and identical per-block content; only the two arms are laid out `[aux][error]` instead of January's `[error][aux]` |
| t6 | t5 with the arms mirrored: `if (A&&B&&C) { if (aux()) return TRUE; } else { error; } return FALSE;` | byte-identical to t5 — source arm order does not move the layout |

Every gate was 17 exact / 2 residual / 0 unwritten with the guard passing; no sibling row changed in any of the six.

### New measured VC7 facts

1. **FALSE-initializer register pin.** `boolean result = FALSE;` declared ahead of a run of calls makes VC7 keep the constant
   0 in a callee-saved register for the whole function and spell later null tests as `cmp [global], ebx` (t2: 144 bytes
   versus 160). A `TRUE` initializer does not.
2. **Branch-normalised boolean return.** January's `test al,al; je <shared xor al,al>; mov al,1` (instead of `setne al`)
   requires a `return TRUE;` **inside** the success arm together with a bottom `return FALSE;` — that is, the FALSE return
   must be the join, not an early return (t5/t6). A result variable with a single bottom return (t1/t3) gives `mov al,1`
   but not the shared false exit; two early `return FALSE;` statements (t4, the park base) give `setne al`.
3. **Arm layout is not source-order driven.** VC7 lays the non-`error()` arm as the fall-through of the last test of an
   `||`/`&&` null chain regardless of which arm is written first and regardless of condition polarity (t1 vs t3, t5 vs t6).

### Disposition and reopen criterion

Real file unchanged; the park stands. The residual class is refined from "instruction-scheduling" to **basic-block
placement**: which of the two arms VC7 makes the fall-through of the third null test. `tg/t5.c` is a strictly closer scratch
body (identical instruction stream to January, blocks swapped) and is offered to the orchestrator only as evidence — it is
not proposed as a zero-credit re-baseline, because its spelling (`return TRUE;` inside an `else if`, `return FALSE;` at the
bottom) is weaker house style than the retained body and laws_w3 R11 rejects re-baselines that are not tied to a reopen.

**Reopen:** a source-level or compiler-level lever that makes VC7 lay the `error()` arm as the fall-through of the last null
test, applied on top of `tg/t5.c`. Everything else in the function is then instruction-identical to January.

## `_rasterizer_sort_external` (176 B, parked) — skipped, hold binds

Target `4f78b8476535a901…` / 14 relocs; park base `30cd227ef42506ff…` / 14; objdiff 80.08772.

`rasterizer_transparent_geometry_obj_jonas_sort_driver_lifetime_20260825.md` records an explicit hold: *"Do not retry the
compiled direct-expression + block-local-pointer + pre-test index family, or split it into spelling variants, without
genuinely new January source or another compiler-authentic artifact that explains both the first-pass ECX/EDX assignment and
the pre-`qsort` relocation order."* laws_w3 **R13** makes that hold binding on this wave.

Diagnostic re-read only (no compile spent). The residual is:

- EBX pushed at entry, where January shrink-wraps it (`push ebx` at `+0x7f`, `pop ebx` at `+0xa9`, around the second pass);
- the first pass forms the group pointer in ECX from an EDX base, where ours uses EAX from an ECX base;
- the second pass emits `xor edx,edx` (the index init) **before** the count test, where ours emits it after the test and the
  two hoisted pointer loads — this is laws_w3 **A29(e)** (loop init before the result test);
- the `qsort` first argument is loaded into EAX between `push count` and `push base`, where ours loads it into EDX before the
  push sequence.

A29(e) therefore fires, but on a shape the hold forbids: the 2026-08-25 candidate already achieved both the shrink-wrap and
the pre-test index (68 → 58 differing bytes) and was reverted, and the only untried spelling — a plain `for` loop for the
second pass, as in HCEA `rasterizer_sort_external.c` — is a spelling variant of that same family, which the hold names and
which the closure-freshness audit already declares exhausted ("HCEA's semantic loop topology and allocation reachability
alone are now exhausted for this source family"). No shape spent.

**Reopen:** January source/PDB local records, or a same-compiler donor that explains **both** the first-pass ECX/EDX
assignment and the late `qsort` base load. A29(e) alone does not lift the 2026-08-25 hold.

## Evidence used

January split COFF bytes, relocations and `.bss`/`.rdata` labels; `tools.campaign.alndiff`, `tools/campaign/gate.py`,
`scratch/parkcheck.py`, `tools/fake_match_scan.py`; `config/parked.json`; the object's prior ledgers (result-lifetime
20260828, sort-driver lifetime 20260825, codex and HCEA checkpoints, closure-freshness audit 20260830); HCEA
`rasterizer_transparent_geometry_initialize.c` and `dx9/rasterizer_sort_external.c` for topology only. No supplied binary was
executed. Worker notes: `scratch/workers/w3c_rasterizer_lights_shader_transparent_chicago_preprocessor_etc.md`.
