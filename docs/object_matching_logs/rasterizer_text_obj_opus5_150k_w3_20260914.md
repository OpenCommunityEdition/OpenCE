# rasterizer_text.obj: Opus5 150K house-clean lane, wave w3 (2026-09-14)

Unit `source/rasterizer/rasterizer_text`. The worker owned only `source/rasterizer/rasterizer_text.c`.

## Result

| gate (guard `_point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (real file at lane HEAD `403108e28`) | 11 | 3 | 0 |
| final real file | **12** | 2 | 0 |

Newly strict EXACT (not aliased, not in the accepted ledger or the 100K manifest):

| function | padded | relocs | normalized sha16 |
| --- | ---: | ---: | --- |
| `_cache_hardware_format_character` | 896 | 74 | `c8c82d6f962bf513` |

It was a structural row at baseline (`size 816!=896, relocs 68!=74`).

Checks on the final file:
- Row-by-row diff against the baseline listing: only this row changes (residual -> EXACT). All 11 baseline-exact rows stay exact, including the two draw callbacks whose EDI private ABI depends on this body.
- The `_point_from_line3d` guard passes. The unit has no parks.
- Owner census against `build/base` and `build/split`: no candidate-only or base-only owner. The literal COMDATs `string` and `__real@3f800000` are candidate-only against the target, as they already were at base.
- `tools/fake_match_scan.py`: 0 leads, before and after.

## `_cache_hardware_format_character`

### What was blocking it

The w1 ledger established the following:
- January inlines the static `flush_hardware_character` (its line-610 assert survives as `test esi,esi`) only in the ring-full block. Both eviction loops call it out of line (+0x176, +0x1ee).
- The landed w1 body kept all three calls out of line (816/68).
- The only exact w1 probe wrapped the ring-full call in a fixed-condition `do { ... } while (FALSE)`, which is inadmissible.
- Its reopen criterion asked for a genuine loop-like source construct that explains the ring-full inline.

### New measured VC7 13.00.9254 law: FOR-INCREMENT FREQUENCY

Lab TUs `scratch/workers/w3_rasterizer_text_rasterizer_xbox_shadows_etc/lab1.c` to `lab5.c` were compiled with this unit's flags. Each has the same 96-byte callee and was checked with `labcensus.py`. The law:

- **Inflating loops.** A `for` loop with an increment clause inflates the estimated frequency of the code after it. A top-level `if (ring full) flush(...)` stays out of line on its own, but is inlined when it follows `for (y = 0; y < h; y++)` or `for (; r != w; r = (r + 1) & 255)`.
- **Loops that do not inflate.** `while`, `do`, guarded `do`, `for (;;)` with a break, and `for (; c; )` without an increment leave the following site out of line.
- **Surviving a join.** The inflation survives an enclosing `if` join: `if (c) { for-inc }` followed by `if (ring) flush` inlines.
- **Compounding.** It compounds over consecutive loops. Inside an `else` arm, one for-inc loop before `if (ring) flush` is not enough (out of line). Two for-inc loops, each inside its own `if`, are enough (inline).
- **Loop bodies.** The bodies of such loops stay cold when the call sits behind an in-body `if (...) break;` inside an enclosing `if` (lab5_e: out of line).

This refines the action_vehicle w1 observation, "loop spelling controls post-loop inline frequency". It also explains every w1 probe: the w1 `for` probes had no increment clause, and while/do spellings do not inflate.

### Source recovered

Both eviction loops are the ordinary ring walk:

```c
for (;
	hardware_character_cache.read_index != hardware_character_cache.write_index;
	hardware_character_cache.read_index = (hardware_character_cache.read_index + 1) & (MAXIMUM_HARDWARE_CHARACTERS - 1))
{
	hardware_character = &hardware_character_cache.characters[hardware_character_cache.read_index];
	if (hardware_character->y0 <= 0)   /* second loop: y0 < y0 || y0 >= y1 */
		break;
	flush_hardware_character(hardware_character);
}
```

**Why this spelling:**
- **Same loop bytes.** It emits byte-identical loop code to the former guarded do-while. `break` skips the increment in both forms, so the semantics are identical.
- **Inlining.** The increment clause makes the later ring-full site hot, so VC7 inlines `flush_hardware_character` there. The loop-body calls stay out of line.

**Three statement facts from w1, proven here in the admissible context** (each form without its fact is residual):
- (a) The row-wrap block stores `x0 = 0;` before `y0 += maximum_character_height;`. January materialises the zero before the y0 add, and HCEA's statement order agrees (tx_f5 without it: 1 block).
- (b) `short next_write_index = (write_index + 1) & (MAXIMUM_HARDWARE_CHARACTERS - 1);` is compared with `read_index`. January's `inc dl; and edx,0xff; cmp dx,ax` is a short-typed 16-bit compare (tx_f2 without it: 17 blocks).
- (c) `hardware_character = &characters[write_index];` precedes the `hardware_character_index` store. January's `movsx ebx,ax` comes before `mov [edi+0xc],ax` (tx_f3 without it: 8 blocks).

**What the original programmer meant:** walk the ring from the read index, evicting glyphs whose row band is invalidated, then evict the single oldest glyph when the ring is full.

### Shapes (new family, 5)

| shape | result |
| --- | --- |
| tx_w1: ring-full as `while (next_write_index == read_index)` | inlined, but the back edge stays; 896/73, not January |
| tx_f1: eviction loops as for-increment | ring-full inline; 896/75 [relocs, sha] |
| tx_f2: f1 + (a) | residual |
| tx_f3: f1 + (a) + (b) | residual |
| **tx_f4 / tx_final: f1 + (a) + (b) + (c)** | **EXACT (landed)** |

The landed diff is `scratch/workers/w3_rasterizer_text_rasterizer_xbox_shadows_etc/rasterizer_text_landed.diff`.

## `_rasterizer_draw_string` / `_rasterizer_draw_unicode_string` (512 B each, sha): not landed

- **What differs:** only the x87 schedule of `map_texture_scale[0]`.
  - January computes and stores `.i` before the height `movsx`, then spreads `.j` across the meter, point-sampled, blend and map stores.
  - Ours spreads `.i` and computes `.j` tight at the end.
  - Integer stores, the merged csmemset stack adjust and the `[ebp-4]` temp reuse are identical.
- **Already tried in w1:** 13 byte-invariant shapes (statement orders, uncast division, `set_real_vector2d`, split `map_scale`).
- **Laws checked:** A4, A11, A12–A14, and the x87 operand-order, phi and L-A laws. None predicts a spelling, so no new shape was compiled.
- **Classification:** class D x87 scheduling tie.
- **Reopen criterion (unchanged):** new evidence for the parameter-block fill spelling.

## Proposals

- **Law for WORKER_BRIEF section 9:** the FOR-INCREMENT FREQUENCY law above, with lab files lab1-5 and census script `labcensus.py` in the scratch directory.
- **Headers and config:** none needed. No header or config change is required.
