# `rasterizer_transparent_geometry.obj`: next-150K lane, Tier B wave (B9)

Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, HEAD `6d6529d98`. Scope: `source/rasterizer/rasterizer_transparent_geometry.c`.
The only target was `_rasterizer_transparent_geometry_initialize` (149 meaningful bytes, parked).
Worker notes: `scratch/workers/tB_rasterizer_transparent_geometry.md`. Scratch: `scratch/workers/tB_rasterizer_transparent_geometry/`.

## Result

**Park reopen proposed. The real file is unchanged.** The parked body stays locked, so `git diff --stat` on the file is empty.

The scratch candidate `scratch/workers/tB_rasterizer_transparent_geometry/v1.c` is the current real file with only this body replaced.

| gate (`--all --forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| real file (baseline and final) | 17 | 2 | 0 |
| scratch candidate v1.c | **18** | 1 | 0 |

- **Target row:** strict EXACT, 160 bytes, 16 relocations, sha `63251d0d8cc730b0`, identical to January.
- **Other rows:** none changed; all 17 exact rows stay exact.
- **Guard:** passes.
- **Parkcheck:** drift on this park only, to the target hash, as a reopen expects. `_rasterizer_sort_external` is unchanged.
- **Owner census (v1.obj against build/base):** identical owner sets.
- **fake_match_scan:** 0 leads.
- **`CL /Zs /W3`:** the same 12 header-only warnings as HEAD.

## Census (written before any shape)

- **tinfo:** 160 bytes, 16 relocations. There are 51 instructions on both sides.
- **Frame:** neither side has a frame or any `[ebp±N]` slots. `frameslot.py` reports NO-FRAME / CLEAN.
- **Relocations by target:** the multisets are identical: `_debug_malloc` ×3, file literal ×3, error literal, `_error`, `_rasterizer_transparent_geometry_initialize_aux_buffer`, and five `.bss` globals. The only difference is the `.bss` defined-vs-COMMON spelling, which every exact sibling shares.
- **Lens 1 (F4 STRUCT-COUNT):** still holds (40 vs 39 instructions).
- **Lens 2 (F5 SETCC-DEFECT):** still holds.

## Why the implied change does not close it

The implied change was to return TRUE from the success arm. Wave w3c had already measured exactly that as shapes t5 and t6. Those bodies are instruction-identical to January but lay the blocks out as `[aux][error]`, where January has `[error][aux]`. Running F5 on t5 gives DOWNSTREAM.

So the setcc lens is satisfied. What remains is where the blocks go:
- January keeps the error arm as the fall-through of the last null test (`jne 0x89`).
- The shared `xor al,al; ret` sits directly after the error call.
- The aux block comes after it and branches **backward** into that FALSE exit (`je 0x86`).

## New evidence

### Law L-SINGLE-TEST (micro-TU lab, this unit's exact cflags)

The lab is `scratch/workers/tB_rasterizer_transparent_geometry/lab/lab.py`. It is an instrument only, never source.

**VC7 keeps an if-arm in place only when a single test selects it.** Here "in place" means the arm is the fall-through, its FALSE exit comes next, and the other path follows after that. When the if-condition is a chain of two or more terms joined by `||` or `&&`, VC7 moves the arm to the function tail.

The model reproduces our current output exactly. In this table, `A`, `B` and `C` stand for the three allocation pointers, and `err` for the error call.

| probe | spelling | layout |
| --- | --- | --- |
| a0 | w3c t5 | ours `[aux][error]` |
| a1 | park base | ours: setne, error at the tail |
| a2 | a0 with `long` globals | same as a0, so pointer typing is not the cause |
| **a3** | **single test `if (!groups2) {err} else if (aux()) return TRUE; return FALSE;`** | **January** |
| b3 | two-term chain | error at the tail |
| b1 / b2 | `!(A&&B&&C)` / `== NULL` | same as a0 |
| v4 / v10 | else-block / result flag with the chain as the condition | error at the tail |
| **v1** | **`success = A&&B&&C; if (!success) {err} else if (aux()) return TRUE; return FALSE;`** | **January; every offset identical** |
| **w5** | **`success = A&&B&&C; if (!success) err; success = success && aux(); return success;`** | **same as v1** |
| **y1** | **`boolean success = TRUE; if (!A\|\|!B\|\|!C) success = FALSE; if (!success) {err} success = success && aux(); return success;`** | **same as v1** |
| **y2** | **`boolean success = TRUE; if (!A\|\|!B\|\|!C) { err; success = FALSE; } success = success && aux(); return success;`** | **same as v1** |
| w2 / w3 / w4 | returned flag with a FALSE store path | 0 pinned in EBX (w3c fact 1) |
| w6 | early returns | setne |
| x1 | `return success && aux();` | 32-bit `xor eax,eax` / `mov eax,1` |

### Same-author donor (strict EXACT)

`rasterizer_xbox.c` `__rasterizer_initialize` (2352 B) is the January caller of this function and is strict EXACT. It uses the same idiom and the same message family:
- `if (!global_d3d_texture_sun_glow_secondary || !global_d3d_surface_sun_glow_secondary) success = FALSE;`
- `if (!success) { error(_error_silent, "### ERROR failed to create offscreen surface(s)"); }`
- `success = success && rasterizer_memory_pool_initialize() && ... && rasterizer_transparent_geometry_initialize() && ...;`
- `return success;`

Its January bytes at +0x712..+0x74b lay the `if (!success)` error arm in place under one test.

### Exact corpus instance of the January topology

ui_widget `_string_has_icons_to_draw` is strict EXACT and shows the same topology:
- a single `if (!string)` assert test with `jne` over the in-place failure block;
- `xor al,al; pop ebx; ret` directly after the failure block;
- a loop `je 0x25` that branches backward into that exit.

Corpus scans (read-only evidence) are `errscan.py`, `backscan.py`, `orscan.py` and `donorscan.py`, with their `.txt` outputs.

## Shape (1 of 5 used)

S1 (`v1.c`) is the y2 spelling:

```c
boolean rasterizer_transparent_geometry_initialize(
	void)
{
	boolean success = TRUE;

	/* the three debug_malloc allocations and the two count clears, unchanged */

	if (!transparent_geometry_groups ||
		!transparent_geometry_group_sorted_indices ||
		!transparent_geometry_groups2)
	{
		error(_error_silent, "### ERROR failed to allocate transparent geometry buffer");
		success = FALSE;
	}

	success = success && rasterizer_transparent_geometry_initialize_aux_buffer();

	return success;
}
```

Result: strict EXACT in the whole-TU gate.

- **Meaning:** allocate the three group buffers. If any allocation fails, report the silent error and fail. Otherwise initialise the Xbox auxiliary buffer. Return the overall success.
- **Idiom:** this is the caller's own success-accumulation idiom.
- **Laws applied:** A3 single-exit, with one bottom return of a genuine result flag, and w3c fact 1: a TRUE initializer stays out of the EBX zero pin.
- **House rules:**
  - no casts, filler, dead or redundant stores, carrier, or operand permutation;
  - `success = FALSE` is read by the `&&`;
  - declare+initialize together;
  - an explicit single return.
- **Alternative:** y1 is the donor's verbatim split form and is byte-identical in the lab. It was not gated on the whole TU. y2 was preferred because this function has only one failure source before the report.

## Park-reopen proposal

- **Unit / function:** `source/rasterizer/rasterizer_transparent_geometry` :: `_rasterizer_transparent_geometry_initialize`, 160 padded / 149 meaningful.
- **Candidate:** `scratch/workers/tB_rasterizer_transparent_geometry/v1.c`, which is the real file plus this body only.
- **Park criterion:** "Reopen only for authoritative January source/local records or a natural same-compiler donor that explains this block order."
- **Why the criterion is met:**
  - The block order is now explained by a measured law (L-SINGLE-TEST).
  - A natural, strict-EXACT, same-author, same-subsystem donor uses the spelling that produces it: the function's own January caller, `__rasterizer_initialize`.
  - The park's premise, that no natural result-lifetime or error-path family reaches it, is refuted: none of the recorded families tested the allocation result through one flag.
- **Orchestrator action:** remove the park entry and land the v1.c body in the same commit, then run the full ninja build and the stable-verdict diff.

## Not in scope

`_rasterizer_sort_external` (176, parked) was outside this wave's single-function scope. It was not touched, and its park is unchanged.
