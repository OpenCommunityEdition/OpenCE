# rasterizer_text.obj: Opus5 150K house-clean lane, wave w1 (2026-09-14)

Unit `source/rasterizer/rasterizer_text`. The worker owned only `source/rasterizer/rasterizer_text.c`.

## Result

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (real file at lane base `12f7375d4`) | 8 | 6 | 0 |
| final real file | **11** | 3 | 0 |

These rows are newly strict EXACT. None of them is aliased, and none is in the accepted ledger or the 100K manifest:

| function | padded | relocs | normalized sha16 |
| --- | ---: | ---: | --- |
| `_rasterizer_draw_character` | 240 | 3 | `b99fef50df849db1` |
| `_rasterizer_draw_character_with_dropshadow` | 368 | 4 | `31edbdb0f9c11a2f` |
| `_rasterizer_text_cache_initialize` | 144 | 13 | `2fa3158fdf45b82f` |

Checks on the final file:
- All 8 baseline-exact rows are still exact.
- The `_point_from_line3d` guard passes.
- The unit has no parks.
- Owner census against `build/base` and `build/split`: no new code, data, BSS or COMMON owner. The only candidate-only literal COMDATs (`string`, `__real@3f800000`) already exist in base.
- `tools/fake_match_scan.py`: 0 leads, before and after.

## Evidence and mechanisms

### 1. The private ABI of `cache_hardware_format_character` (EDI) comes from the loop spelling

- January passes `font_character` to the static `cache_hardware_format_character` in EDI: at +0x2d it reads `mov ax,[edi+0xc]` without loading EDI first. Both draw callbacks load EDI.
- The baseline body wrote both eviction loops as `while`. VC7 then auto-inlined `flush_hardware_character` into both loops and pinned `-1` in EDI. That pushed `font_character` to EBX, and both callers mismatched.
- January calls `flush_hardware_character` out of line in both loops (relocations at +0x176 and +0x1ee).
- Writing the loops as `if (read_index != write_index) do { ... } while (read_index != write_index);` keeps those calls out of line. The later HCEA build has the same shape. This moves the ABI to EDI.
- Probe `t5` confirmed the two loop regions and the first `if` equal January modulo registers.
- Every `while` or `for` spelling re-inlines the calls: probes v0, v2, F3, F4, F6, W1.

### 2. Dropshadow offsets

This follows the x87 operand-order law.
- January stores both shadow offsets (1.0) before it computes `shadow_color`, and it emits `fld left; fadd x_offset`. So the offsets are created first.
- At the loop tail, January resets y before x. That is the store order of a chained assignment.
- The landed source declares and initializes `real x_offset = 1.0f; real y_offset = 1.0f;` first and resets with `x_offset = y_offset = 0.0f;`.
- Shape D1, with separate reset statements, is not exact.

### 3. Cache initializer

This follows the single-exit law. It is new compiler-provenance evidence that meets the reopen criterion of `rasterizer_text_obj_jonas_cache_initialize_rejection_20260826.md`.

- Probe I1 used a `boolean success = FALSE` result that is returned once. VC7 homes it in EBX (`xor bl,bl`) and returns it unchanged on the failure edge. That is the mirror image of January's `mov ebx,1`.
- So January's EBX=1 is a result initialized to TRUE:
  - it is unchanged on success, where the `initialized` store, the return and the assertion's TRUE all use `bl`;
  - it is set FALSE on each failure edge after the error report.
- The landed body is I5:
  - `boolean success = TRUE;`
  - each failure arm (bitmap allocation failed, rasterizer registration failed) does `error(...); success = FALSE;`
  - a single `return success;`
- VC7 tail-merges the two identical failure tails into January's single error call.
- None of the five forms the earlier ledger lists as do-not-repeat was reused.
- Rejected shapes:
  - I3: flag, then store;
  - I4: `if (!success) error` after flag-only else arms, which differs by one FALSE-store placement;
  - I6: `initialized = success`.

## Remaining residuals

### `_cache_hardware_format_character` (896 B target; landed body 816/68)

- **What differs:** January inlines `flush_hardware_character` in exactly one place, the ring-full block, and keeps its `test esi,esi` assertion there.
- **What the probes show:** VC7 13.00.9254 auto-inlines this 96-byte static only at hot sites:
  - inlined at:
    - `while` and `for` bodies;
    - unguarded `do` bodies;
    - top-level code after a `for` loop;
    - the body of `do { } while (FALSE)`;
    - a constant one-trip `while`;
  - never inlined at:
    - entry level (t8);
    - inside an `if` (t3, B1–B9);
    - inside a single-use helper (t6, H1);
    - with `__inline` (t4, t4b);
    - after `while` or `for` loops nested in `if`s (G*, P*, Q1–Q3).
- **Exact probe:** `scratch/workers/rasterizer_text/cache_hardware_d11_inadmissible_probe.c` matches January strictly (sha `c8c82d6f962bf513`). It is the landed file plus:
  - (a) in the first row-wrap `if`, `x0 = 0;` before `y0 += maximum_character_height;`;
  - (b) `short next_write_index = (write_index + 1) & (MAXIMUM_HARDWARE_CHARACTERS - 1);`, compared with `read_index`, which gives the byte increment and the word compare;
  - (c) `hardware_character = &characters[write_index];` before `font_character->hardware_character_index = write_index;`;
  - (d) the ring-full flush call wrapped in `do { ... } while (FALSE);`.
- **Why it was not landed:** (d) is a fixed-condition construct, and no genuine January macro accounts for it. (a)–(c) were not landed either, because a residual body may change only when it becomes exact.
- **Why the body still changed:** the body change that was landed (the do-while loops) is required for the two exact callers above. It is January-faithful in where the calls go out of line. Its size moved from 896 to 816 because the ring-full inline is still missing. This is disclosed for orchestrator review.
- **Reopen criterion:** authentic evidence of a loop-like scope around the ring-full flush call, such as a genuine statement macro or a real one-trip loop. Then apply (a)–(d) equivalents.

### `_rasterizer_draw_string` and `_rasterizer_draw_unicode_string` (512 B each, sha only)

- **What differs:** only the x87 schedule of `map_texture_scale[0].i/.j`. January stores `.i` immediately and spreads `.j` over the integer stores. The candidate spreads `.i` and computes `.j` last.
- **What was tried:** 13 shapes, all byte-invariant: statement orders O1–O6 and X3–X5, the uncast division X1, and Y1–Y3 (`set_real_vector2d` and split `map_scale` stores). Function order in the file is also inert (v1). No flag lever was admissible.
- **Classification:** x87 scheduling tie.
- **Reopen criterion:** new evidence about how the parameter block is filled, for example a January sibling whose same fill is exact.

## Proposals

None: no header or config change is needed. The three landed functions keep their existing names and linkage.

## Scratch

- Worker notes: `scratch/workers/rasterizer_text.md`.
- Candidates and probes: `scratch/workers/rasterizer_text/`.
  - `final_landed.c` is identical to the real file.
  - `b_*.c` and `vars*.json` hold the probe matrix.
