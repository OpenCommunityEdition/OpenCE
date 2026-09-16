# `effects.obj` Opus5 250K house-clean lane, wave w3d: structural pass (2026-09-15)

## Scope and provenance

- Translation unit `source/effects/effects.c`, target `build/split/source/effects/effects.obj`.
- Compiler VC7 13.00.9254 (XDK 3911). No header, config, flag or tooling change.
- Lane `opus/250k-house-clean-20260915` (worktree `opus5-150k-house-clean-20260914`), worker `w3d:effects`.
- Assigned TARGETs (both parked and locked): `_effect_generate_particles` (1888, `[size 2048!=1888,
  relocs 55!=46, sha]`) and `_effect_allowed_by_environment` (128, `[size 112!=128, sha]`).
  `_effect_generate_part` was explicitly out of scope this wave.
- Every experiment ran on scratch copies under `scratch/workers/w3d_effects/`.
  **The production file is unchanged** (`git diff --stat -- source/effects/effects.c` is empty).
- Worker notes with the mandatory censuses and per-function blocks: `scratch/workers/w3d_effects.md`.
- Evidence: the January target COFF (disassembly, relocations, jump tables, frame slots); the ledgers
  `effects_obj_opus5_150k_w2_20260914.md`, `effects_obj_opus5_100k_waveD_20260914.md` and
  `effects_obj_opus_reconciliation_20260904.md`; `config/parked.json`; the later HCEA per-function sources
  (semantics only); effects.c's own exact functions for idiom precedent.

## Baseline (real file, HEAD)

`== exact 38  residual 3  unwritten 0  (of 41 listed)`. Point guard passes; park drift 0/3.

| residual (all parked) | target | base | tag |
|---|---|---|---|
| `_effect_allowed_by_environment` | 128/10 | 112/10 | size, sha |
| `_effect_generate_part` | 992/27 | 976/28 | size, relocs, sha (not a target) |
| `_effect_generate_particles` | 1888/46 | 2048/55 | size, relocs, sha |

## Census 1 - `_effect_allowed_by_environment`

**Frame census.** January: `push ebp; mov ebp,esp; push ecx` - exactly one 4-byte cell, read once at
`0x5f` (`mov al, byte ptr [ebp-1]`) and **never written on any path** (the whole 59-instruction body was
disassembled). Ours: frameless, no `[ebp-N]` at all.

**Control flow (jump-table order = case value order).**

```
ja 0x45                    -> default
case 0 anywhere @0x13 : mov al,1
case 1 air      @0x19 : call scenario_location_underwater; neg al; sbb al,al; inc al
case 2 water    @0x2f : call scenario_location_underwater
case 3 vacuum   @0x3f : xor al,al
default         @0x45 : display_assert(NULL, file, 888, TRUE); system_exit(-1);
                        mov al,[ebp-1]           <- returns an UNINITIALISED stack byte
```

**Relocation census.** 10 vs 10, every target identical; only the four internal jump-table entries differ -
January `[0x13,0x19,0x2f,0x3f]` (four distinct blocks), HEAD `[0x5b,0x16,0x2e,0x5b]` (case 0 and case 3
merged by the `boolean allowed = environment == _effect_environment_anywhere;` initializer). **No call or
global is missing on either side.**

**Structural defect derived.** The 16-byte gap is one extra exit block plus the 4-byte frame. January has
five distinct exits; HEAD has four, because its default arm returns a value that is neither `TRUE` nor
`FALSE` but a load of a never-written local.

## Census 2 - `_effect_generate_particles`

**Relocation census by target** (`relocdiff --count-by-target --allow-structural`): the whole +9 is one
inlined copy of the environment helper.

| target | January | HEAD | delta |
|---|---:|---:|---:|
| `_effect_allowed_by_environment` (call) | 1 | 0 | -1 |
| `_scenario_location_underwater` | 0 | 2 | +2 |
| effects.c `__FILE__` literal | 1 | 2 | +1 |
| `_display_assert` | 1 | 2 | +1 |
| `_system_exit` | 1 | 2 | +1 |
| `internal:` (table base + 4 entries) | 0 | 5 | +5 |
| all 26 other targets | = | = | 0 |

**Frame census** (`scratch/workers/w3d_effects/slots.py`): January `sub esp,0xe4`, HEAD `sub esp,0xe8`.
The aggregate set is identical, shifted one cell; the extra cell is the home of `count_delta`
(`mov [ebp-0xc8],ebx` @0x12d, reloaded @0x190) which January keeps in EBX while homing `instance` at
`[ebp-0xc]`. That is a register-role swap downstream of the inline (laws_w3 A44).

**Conclusion.** No missing statement, call, local or loop shape: the entire structural gap was one inline
decision.

## Results

Candidate: `scratch/workers/w3d_effects/c1_crlf.c` (CRLF, three hunks against the real file; LF twin
`c1.c`, object `c1_crlf.obj`).

| row | HEAD | candidate c1 | change |
|---|---|---|---|
| `_effect_allowed_by_environment` | 112/10 `[size, sha]` | **128/10 `[sha]`** | size + relocation identity now exact |
| `_effect_generate_particles` | 2048/55 `[size, relocs, sha]` | **1888/46 `[sha]`** | size + relocs now exact, frame exact |
| all 38 exact rows | EXACT | EXACT | unchanged (row-by-row diff) |
| `_effect_generate_part` | 976/28 | 976/28 | untouched |

Neither row becomes strict EXACT, so this wave claims **zero strict bytes**. Both rows are parked, so the
candidate is a **coupled park re-baseline** (laws_w3 "newly ADMITTED": a coupled re-baseline of a still-parked
callee to a strictly closer body when a control proves the caller needs it). The control is u1/v0:
with the same caller body, the helper is out of line only when the callee body changes.

### The three source hunks

1. `effect_allowed_by_environment`: `boolean allowed = FALSE;` and `case _effect_environment_anywhere:
   allowed = TRUE; break;` replace the computed `allowed = environment == _effect_environment_anywhere;`
   initializer. Semantics are unchanged (anywhere TRUE, air `!underwater`, water `underwater`, vacuum and
   invalid FALSE) and the default-deny reading matches HCEA's reconstruction of the out-of-range case.
2. `effect_generate_particles`, attached arm: the file's own idiom
   `short node_index = instance->node_designator; if (node_index != NONE) node_index &= ...;` replaces the
   `?:`. Proven by January 0x3de-0x402, which stores the designator register itself in the NONE arm.
   In-file precedent: effects.c:1672 and effects.c:1804 (both inside the exact-adjacent
   `_effect_generate_part`).
3. `effect_generate_particles`, count computation: a separate `long last_particle_count` statement
   evaluated first. Proven by January 0xd6-0x11d, which calls the integral with `last_event_fraction`
   ([ebp-0xc4]) first and homes that result in a 4-byte slot before calling with `event_fraction`
   ([ebp-0x14]). With this hunk the candidate is byte-identical to January from 0xd6 through 0x11d,
   including the `movzx edx, byte ptr [ebx+edi+0xdc]` addressing HEAD does not produce.

### NEW LAW (w3d-1): the callee body, not the call site, sets a TU-static's inline decision

Call-site levers are inert at this site: respelling the inner `for`-with-increment as a `while` (A23
detector, g1) and adding an `if (count_delta > 0)` nesting level instead of `continue` (inline-frequency
detector, g4) both leave the row at 2048/55 with the helper still inlined. The callee body is decisive and
orders by cost:

| env body | env | generate_particles | generate_parts |
|---|---|---|---|
| per-case `return`s (cheapest) | 112/10 | 2016/55 (inlined) | **688/24 - exact row lost** |
| computed initializer (HEAD) | 112/10 | 2048/55 (inlined) | EXACT (called) |
| constant initializer + one assigning case | 128/10 | 1952/46 (**called**) | EXACT (called) |
| uninitialised local (January, prohibited) | EXACT | 1952/46 (**called**) | EXACT (called) |

Detector for other units: when January *calls* a TU-static at a hot site that we inline, do not respell the
caller - count January's distinct exit blocks in the CALLEE and reproduce them.

### The park's recorded negative is stale

`config/parked.json` for `_effect_allowed_by_environment` says "Simple FALSE initialization and
assignment-after-assert forms also produced 112 bytes but caused both callers to inline the assertion body
and regress broadly". The same entry dates itself by naming "the donor's better 1,616-byte and 592-byte
shapes in both real callers"; `_effect_generate_particles` was re-based to the corrected per-instance loop
nest (2048/55) on 2026-09-14. Against that caller the simple-FALSE-initialization form measures 128/10 with
both callers intact. The assignment-after-assert half of the premise still holds (112, probe e5).

### Why strict EXACT is still owner-blocked

`_effect_allowed_by_environment` reaches EXACT only with `boolean allowed;` left uninitialised so the
default arm returns the never-written home (probe u1, EXACT). That is prohibited by WORKER_BRIEF section 5
(uninitialised read) and fails R19: effects.c's own post-fatal-assert precedent,
`_effect_evaluate_function_integral` (exact at HEAD), returns a DEFINED fallback. HCEA
(`effect_allowed_by_environment.c:4`) corroborates the indeterminate original but is not a licence.

With the safe initializer the residual is a result-storage tie: a definite initializer makes `allowed` live
from entry across two `scenario_location_underwater` calls, so VC7 gives it a callee-saved register
(`push ebx; xor bl,bl; ... mov al,bl; pop ebx`); only the uninitialised local, whose live range starts
inside the arms, produces January's `push ecx` frame and AL-per-arm returns.

`_effect_generate_particles` residual under the candidate: 566 vs 562 instructions (was 622), 75 vs 120
aligned diff blocks. The four extra instructions are `mov [ebp-0xc0],ebx` (count_delta spilled) and the
per-instance `test bx,bx; jle` guard January elides because count_delta stays in EBX - exactly the
"loop guard and register form" the park text already records. Its environment-helper half is resolved.

## Experiment matrix (all scratch)

| probe | change | env | generate_particles | generate_parts |
|---|---|---|---|---|
| v0 | control (= HEAD) | 112/10 | 2048/55 | EXACT |
| e1 | env per-case `return`s | 112/10 | 2016/55 | 688/24 LOST |
| e2 | env `= FALSE` init + anywhere TRUE | **128/10** | **1952/46** | EXACT |
| e3 | env `= TRUE` init + vacuum FALSE | 128/10 | 1952/46 | EXACT |
| e4 | env `= FALSE` init, every case assigns | 112/10 | 2016/55 | EXACT |
| e5 | env default assigns FALSE before the assert | 112/10 | 2016/55 | EXACT |
| e6 | e5 with vacuum falling through | 144/10 | 1952/46 | EXACT |
| u1 | env uninitialised local (PROHIBITED, evidence only) | EXACT | 1952/46 | EXACT |
| g1 | inner `for` -> `while` + increment (A23) | 112/10 | 2048/55 inert | EXACT |
| g2 | node_index idiom | 112/10 | 2032/55 | EXACT |
| g3 | g2 + `last_particle_count` statement | 112/10 | 2048/55 | EXACT |
| g4 | `if (count_delta > 0)` instead of `continue` | 112/10 | 2048/55 inert | EXACT |
| u2 | u1 + g2 | EXACT | 1904/46 | EXACT |
| u3 | u1 + g2 + g3 | EXACT | 1888/46 | EXACT |
| **c1** | **e2 + g2 + g3** | **128/10** | **1888/46** | EXACT |
| c2 | e3 + g2 + g3 | 128/10 | 1888/46 | EXACT |

## Do-not-repeat (added by this wave)

- Call-site levers for the environment-helper inline: loop respelling (A23) and extra `if` nesting are both
  measured INERT in `_effect_generate_particles`.
- `effect_allowed_by_environment` written with per-case `return` statements: it is cheaper to inline and
  costs the exact `_effect_generate_parts` row.
- `effect_allowed_by_environment` with every case assigning explicitly (e4), or with the default assigning
  before the assert (e5): both 112.

## Orchestrator proposals

1. **Coupled park re-baseline (zero strict credit)** of `_effect_allowed_by_environment` and
   `_effect_generate_particles` from `scratch/workers/w3d_effects/c1_crlf.c`, as one object commit.
   New park bases: env 128 / 10 / `8f9a2185534b3801`; generate_particles 1888 / 46 / `c251b0c184dbcdeb`.
   Both are strictly closer (size and relocation count now exact on both rows); `_effect_generate_part`
   does not drift; all 38 exact rows are unchanged.
2. **Park evidence text**, `_effect_allowed_by_environment`: replace the stale sentence about simple-FALSE
   initialization with the e2/e5 measurements above; the reopen criterion (owner ruling (a)) is unchanged.
3. **Park evidence text**, `_effect_generate_particles`: the environment-helper inline decision is resolved
   by a natural safe callee body; the remaining residual is the count_delta-EBX vs instance-EBX role swap
   and the per-instance loop guard.
4. **Owner ruling (a) (unchanged question, now fully quantified):** if a read reachable only when
   `system_exit(-1)` returns is admitted, `scratch/workers/w3d_effects/u3.c` makes
   `_effect_allowed_by_environment` strict EXACT (128 B) and leaves `_effect_generate_particles` at
   1888/46 `[sha]`. Without it, c1 is the ceiling.

## Checks (candidate c1)

Gate `== exact 38 residual 3`, only the two proposed rows change; point guard passes; `scratch/parkcheck.py`
drifts exactly those two parks and leaves `_effect_generate_part` at base; owner census vs `build/base` and
`build/split` is identical (no new candidate-only code/data/BSS/COMMON owner); `tools/fake_match_scan.py`
reports 0 leads, the same as HEAD.

## Final real-file gate

`== exact 38 residual 3 unwritten 0`, identical to the baseline row by row. Point guard passes, park drift
0/3, owner census identical, fake scan 0 leads. `git diff --stat -- source/effects/effects.c` is empty and
`newly_exact` is empty: nothing in this wave is strict EXACT without an owner ruling.
