# rasterizer_xbox_hardware_geometry.obj: Opus5 150K house-clean lane, wave w3 (2026-09-14)

Unit `source/rasterizer/xbox/rasterizer_xbox_hardware_geometry`. The real file is unchanged, and `git diff --stat` is empty.

## Baseline and final

| gate (with guard) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 15 | 2 | 0 |
| final | 15 | 2 | 0 |

Both residuals are parked, and park drift is 0 at baseline and at the end:
- `_rasterizer_vertex_buffer_new`, 368/20
- `_rasterizer_triangle_buffer_new`, 352/24

Owner census and fake scan are unchanged: no candidate-only owner, 0 leads.

## `_rasterizer_vertex_buffer_new` (not landed; 5 shapes)

### New evidence

The January prefix is the D3D creation idiom that the repository already has exact in `rasterizer_xbox.c` (preinitialize, frame_end, present) and in `rasterizer_xbox_hardware_bitmaps.c`:
- The create call is checked with `if (result >= 0) success = TRUE; else { success = FALSE; rasterizer_error(result, "<stringised call>"); }`.
- The returned pointer is checked with `if (!ptr) success = FALSE; if (!success) ptr = NULL;`. January's +0xb3..+0xc2 block and the lock-failure `xor bl,bl; mov [ebp-8],edi` block are both this idiom, with `success` known TRUE at the lock.
- The function ends with a single exit: `if (!success) { csmemset(...); error(_error_silent, ...); } return success;`.

The XDK `IDirect3DVertexBuffer8_Lock` returns a constant `S_OK` (`xbox/include/d3d8.h:2256`).

This barrier-free body contains none of the park body's `_ReadWriteBarrier`, mid-function `return TRUE`, `if (!ptr || !success)` double test, or redundant `locked_vertices = NULL`. It reproduces every instruction through the `csmemcpy` call.

### Shapes

Bodies are in `scratch/workers/w3_rasterizer_text_rasterizer_xbox_shadows_etc/vb_v*.txt`; candidates are `hg_v*.c`.

| shape | result |
| --- | --- |
| v1: idiom body, single exit | 352/20 [size]. The store path merges into the `mov al,bl` return; January tail-duplicates `mov al,1` and frees EBX for `vertices`. |
| v2: v1 plus the house check on the Lock result | Materialises `mov bl,1`, which January does not have. |
| v3: v1 with January's emitted store order (count, offset, base, type, hw) | 352 [size] |
| v4: v3 plus `return TRUE;` after the stores | 368/20 sha `2ec337c00675316c`. This equals w2's barrier-free vt3 fixed point: a 3-instruction tail register tie. January loads count then type, stores count, reloads the D3D buffer into EDX, and never touches EAX. |
| v5: `IDirect3DVertexBuffer8_Lock(...) >= 0 && success` form | `mov bl,1` again |

A diagnostic with a late `success = TRUE` (a redundant store, so never a candidate) was byte-identical to v4. EAX is therefore not reserved by the returned value.

### Classification and reopen

- **Classification:** unchanged. This is an allocation tie in the post-`csmemcpy` store block.
- **Best clean body:** `vb_v4.txt`, at the same envelope and sha as w2's vt3. It is not strictly closer, so no remeasure is proposed (R11).
- **Reopen when:** a spelling makes the store path reuse the register freed by the first store for the D3D-buffer reload. Start from `vb_v4.txt`, and do not repeat field-order sweeps.

## `_rasterizer_triangle_buffer_new` (not landed; 2 shapes)

### January layout

- The single failure block (`xor bl,bl`; csmemset; error; `mov al,bl` return) sits directly after the `global_d3d_device` test.
- The create, lock and store blocks follow it.
- Every later failure edge jumps back into that block.

### Shapes

| shape | result |
| --- | --- |
| tb_v1: idiom body with `if (!global_d3d_device) success = FALSE; else {...}` | 336 [size]. VC7 lays the create block first and the failure block last. |
| tb_v2: the same body with the arms inverted | 352 [sha], but with the same inverted layout |

No law predicts January's block order. The park's `goto` form remains the only layout match, and its store tail has the same register tie as the vertex constructor.

**Reopen when:** a structured spelling places the failure block after the device test. Then re-check the shared store-tail tie.

## Proposals

None. No header or config change is needed, and no park is reopened.
