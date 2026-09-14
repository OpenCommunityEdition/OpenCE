# `xbox_texture_cache.obj` — opus5 150K lane, wave w2 (2026-09-14)

## Baseline and result

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| real file, baseline (guard on) | 19 | 6 | 0 |
| real file, final (unchanged) | 19 | 6 | 0 |
| scratch park-reopen candidate `scratch/workers/xbox_texture_cache_cache_files_windows_etc/tex_final.c` | **22** | 3 | 0 |

All six non-exact rows are parked, so the real file is unchanged and `git diff --stat` is empty. Three parks are
proposed for reopening. The candidate is the current real file with only those three bodies edited; one of the
three bodies is byte-for-byte unchanged. In the candidate:

- no baseline-EXACT row changes;
- the `_point_from_line3d` guard passes;
- the only park drift is the three reopened functions, each moving to the January hash;
- the owner census against the baseline object shows only those three body hashes;
- the fake-match scan reports 0 leads.

## Per-function outcomes

| function | padded | outcome |
| --- | ---: | --- |
| `_texture_cache_locked_block_proc` | 64 | **PARK-REOPEN-PROPOSED** (strict EXACT in scratch) |
| `_texture_cache_build_hardware_format` | 336 | **PARK-REOPEN-PROPOSED** (strict EXACT in scratch) |
| `_texture_cache_start_loading_bitmap` | 224 | **PARK-REOPEN-PROPOSED** (EXACT once its callee ABI is fixed; body unchanged) |
| `_render_inverse_transform_screen_point` | 256 | OWNER-BLOCKED (`_point_from_line3d` policy) |
| `_texture_cache_debug_render` | 784 | OWNER-BLOCKED (same policy, plus a data symbol name) |
| `_texture_cache_steal_memory` | 224 | NOT-LANDED (ESI/EDI tie, 4 shapes) |

### `texture_cache_locked_block_proc`

```c
return !texture->loaded ||
	IDirect3DBaseTexture8_IsBusy(&texture->hardware_format);
```

**What January does.** January tests `eax` after `IsBusy`, then:
- on the false path it returns the known-zero `EAX`;
- on the true path it returns `mov eax,1`.

This is the int-typed 0/1 value of a logical `||`. The 2026-09-05 Unicorn harness had already recorded that January
normalises the whole of `EAX` to 0/1, which the byte-returning `if`/`return FALSE` form does not. Callers still test
only `AL`, so the boolean ABI is unchanged.

**Why the park criterion is met.** The park asked for compiler-context evidence explaining the return
materialisation. The `||` expression is that explanation.

**Shapes tried:**
- the single-exit flag form stays residual;
- `!(loaded && !IsBusy)` is also exact, but `||` is the natural spelling.

### `texture_cache_build_hardware_format`

The body was rewritten as follows:
- In the non-linear branch, `Format` is assigned as one expression built from the XDK `D3DFORMAT_PSIZE/VSIZE/USIZE/FORMAT/DIMENSION/MIPMAP_SHIFT` constants, and `Size = 0` is assigned afterwards. This is the same order the linear branch already uses.
- In the linear branch, `Size` is spelled with `D3DSIZE_PITCH_SHIFT` and `D3DSIZE_HEIGHT_SHIFT`. The previous spelling used `D3DSIZE_HEIGHT_SHIFT` twice.

VC7 lowers the constant shifts to January's Horner chain (`shl 4/4/12/4/4`). The chain accumulates in the
pinned-zero register, and the private register ABI flips to January's: texture in `EDI`, zero in `EBX`.

**Shapes tried:**

| shape | build_hardware_format | caller `start_loading_bitmap` |
| --- | --- | --- |
| hf1: `Size` before the expression | residual | EXACT |
| hf2: `Format` then `Size` | EXACT | EXACT |
| hf3: hf2 with named constants | EXACT | EXACT |
| hf4: hf3 plus the linear `Size` fix | EXACT (chosen) | EXACT |

### `texture_cache_start_loading_bitmap`

The body is unchanged. January passes `&texture->hardware_format` to the private static `build_hardware_format` in
`EDI`. The residual was entirely the callee's private ABI (methodology step 5: solve private helpers from their ABI).

### `render_inverse_transform_screen_point` and `texture_cache_debug_render`

**The helper.** January loads `global_zero_vector3d` once, into the screen-position register, and adds it to
`frustum.world_vertices[4]`. That is the inline expansion of:

```c
point_from_line3d(&render.frustum.world_vertices[4], global_zero_vector3d, 1.0f, world_position)
```

Replacing only that statement (r7) makes the helper strict EXACT and flips its ECX/EDX ABI. But it emits a
`_point_from_line3d` COMDAT, which the lane forbids. Plain-C imitations of the inline temporary did not flip the
ABI. Those were: origin declared last, a block-scoped `real_vector3d const *offset`, and a result-alias block (the
last is inadmissible anyway).

**The renderer.** On top of r7, one more change makes the renderer's bytes identical to January (d1):

```c
tab_stops[1] = rasterizer_globals.reserved04.frame_bounds.x0 + 110;
```

January adds 110 to `AX` without reloading `tab_stops[0]`. With that change, the stack packing and the loop register
rotation also match. The only remaining gate tag is `[reloc-identity]`: January names the pointer array
`_bss_004d1198`. With `--alias texture_cache_debug_bitmaps=bss_004d1198`, both functions are EXACT.

The proof files are `r7.c` and `d1.c`.

### `texture_cache_steal_memory`

January computes the writable guard address late (`lea esi` after `lruv_resize`) and the end guard early, spilling
it into the dead parameter home. That rules out a named early writable local.

**Shapes tried:**
- sm1, a named writable local used for the end guard: structurally worse;
- sm3, no `stolen_size` local: worse;
- sm4, end-guard operand order: same fixed point;
- sm5, named writable local assigned after the resize: same fixed point.

## Orchestrator proposals

1. **Owner ruling on `_point_from_line3d` COMDAT policy.** This ruling now also gates this object's helper (256) plus
   `texture_cache_debug_render` (784). It very likely gates the identical sound-cache pair too; see
   `xbox_sound_cache_obj_opus5_150k_w2_20260914.md`.
2. **symbols.json rename, together with 1.** Line 22743 changes from:

   `{ "file_offset": 5050776, "flags": 0, "name": "_bss_004d1198" }`

   to:

   `{ "file_offset": 5050776, "flags": 0, "name": "_texture_cache_debug_bitmaps", "static": true }`

   Evidence: `debug_render` stores `bitmap_data` pointers at +0 with index < 0x580 and qsorts them. The 20260904
   census found no cross-TU reference. The name is descriptive; no original record exists.

## Reopen criteria

- **Helper / debug_render:** a ruling admitting `point_from_line3d` here, plus the rename.
- **steal_memory:** authoritative locals, or a same-compiler donor for the late CSE temp's callee-saved register.

Worker notes: `scratch/workers/xbox_texture_cache_cache_files_windows_etc.md`.
