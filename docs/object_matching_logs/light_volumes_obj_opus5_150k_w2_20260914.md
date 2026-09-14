# `light_volumes.obj`: Opus5 150K lane, wave w2 (2026-09-14)

## Result

Nothing landed, and `source/objects/widgets/light_volumes.c` is unchanged. The unit gate is still exact 9 / residual 1 / unwritten 0.

The parked `_light_volume_render` was re-investigated in scratch only. That produced three new pieces of source-shape evidence, but no exact or closer body.

| | padded / relocs / frame / sha16 |
| --- | --- |
| January | 912 / 47 / 0xA4 / `2e43dbc111ad1255` |
| retained (park base) | 928 / 47 / 0xA8 / `a4e273c793988214` |
| best structural scratch probes | 928-944 / 48 / 0xAC |

- **Park drift:** none.
- **Guard:** passes.
- **Siblings:** 9/9 exact in every probe.
- **Fake-match scan:** 0 leads.

## New evidence (scratch: `scratch/workers/render_particles_rasterizer_xbox_decals_etc/lv*.c`)

### 1. The loop index is a `short`

January's loop header matches, apart from slot numbers, with:

```c
short count;
short sprite_index;

for (sprite_index = 0; sprite_index < count; sprite_index++)
{
	real offset_fraction = pow1((real)sprite_index / (real)(count - 1), frame->offset_exponent);
	...
}
```

VC7 turns the `short` index into a 32-bit shadow index and a 16-bit trip counter. The January header is:

```
mov ax,[ebx+6E]; test ax,ax; jle; movsx ecx,ax; dec ecx; fild; fstp [divisor]; movzx edi,ax
... inc ecx; dec edi; mov [ebp-C],ecx; jne
```

Other loop forms do not reproduce it:
- the retained `do {} while (--count)` form emits `test di,di`;
- a `long` index emits `cmp edi,eax; jl`;
- an extra `if (count > 0)` guard pushes `pow1` out of line (848 B), per the inline frequency law.

HCEA independently renders the index increment as `i = (int16_t)(i + 1)`.

### 2. `delta` comes from `vector_from_points3d`

January stores `delta` in memory (`[ebp-18..-10]`) and shares those slots with the loop's `position`. It also emits the 1.0f immediate store between the delta.x `fsub` and `fstp`.

- The helper `vector_from_points3d(&render.camera.position, &marker.matrix.position, &delta)` reproduces both.
- The retained hand-expanded subtraction keeps `delta` on the x87 stack instead.

### 3. `function_value` has a 1.0f default

January writes 1.0f into the slot whose address is passed to `object_get_function_value` (`[ebp+8]` at +0xD3, just before the far-fade test). This is a genuine `function_value = 1.f` default placed after the parallel factor; the 12-cell grid variant `lvG_TP` reproduces that position.

## Measured as inert or wrong

- **Canonicalised by VC7 (hash unchanged):**
  - `dot_product3d` argument order;
  - declaration order;
  - scoping of `function_value`;
  - an intensity-accumulator spelling (web renaming);
  - two-statement versus nested `pow1` arguments;
  - `pow` versus `power` inside `pow1`.
- **Break `_pow1`:** the ternary and result-variable `pow1` rewrites; they also force it out of line.
- **Wrong NaN predicate:** the if-negate form of ABS (`test ah,5`); January uses `test ah,1`.
- **Wrong operand order:** the HCEA hand-written dot-product order. It loads the global operand first in every term, but January loads the local first in the k and j terms.
- **No effect:** declaring `light_volume_globals` static. It is absent from the PDB publics too, but the renderer does not change.

## Remaining boundary

Two x87 choices remain.

1. **Across the far-fade `if`:**
   - January keeps `|parallel_factor|` on the x87 stack and stores `distance_fade` in memory (immediate store, 47 relocations).
   - VC7 does the reverse: `fld 1.0f` costs one relocation, and `parallel_factor` goes to memory.
2. **The four inlined `pow1` joins:**
   - January merges all four on the x87 stack.
   - VC7 merges three of them through memory, which adds frame slots (0xAC).

Neither choice moved under any statement, declaration or scope permutation.

## Reopen criterion

Reopen on a source-level fact that changes VC7's x87 choice between `distance_fade` and `parallel_factor`, or between x87 and memory merges for the inlined `pow1` results.

Start from `lvJ` / `lvG_TP` (short index, `vector_from_points3d`, `function_value` default), not from the retained body. Do not repeat the probes listed above.
