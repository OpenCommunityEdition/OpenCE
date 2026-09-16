# `player_effects.obj` — opus5 250K house-clean lane, wave w3c (2026-09-15)

## Result

| Measure | Baseline (HEAD) | Final |
| --- | --- | --- |
| Focused gate | 24 exact / 5 residual / 0 unwritten | **25 exact / 4 residual / 0 unwritten** |
| `_point_from_line3d` guard | pass | pass |
| Parks (`scratch/parkcheck.py`) | 3 active | 3 active, **drift 0** |
| Owner census vs `build/base` and `build/split` | clean | clean (empty both directions) |
| `tools/fake_match_scan.py` | 0 leads | 0 leads |

One function closed: **`_player_effect_get_screen_flash`, 800 padded bytes, strict EXACT**
(was `residual 800 [size 816!=800, sha]`). Every other row is byte-identical to baseline; the three parked
bodies (`_player_effect_get_camera_effect_matrix`, `_player_effect_update_camera_impulse`,
`_player_effect_start`) are untouched in the real file.

Source diff: one hunk in `source/effects/player_effects.c` (8 insertions / 6 deletions).

## `_player_effect_get_screen_flash` (800) — EXACT

### The single difference

At `+0x20a` January clears the "just started" flag with one in-place byte RMW,
`and byte ptr [edi+0xe8],0xfe` (7 B). Our build emitted `mov al,[edi+0xe8]` … `and al,0xfe` …
`mov [edi+0xe8],al` (14 B) with `movsx edx, word ptr [edi+0x18]` scheduled **between** the load and the `and`.
The extra 7 bytes crossed the 16-byte alignment, giving 816. Everything else — all 219 instructions and all 43
relocation identities — already matched.

### What the earlier waves had refuted

The w1 ledger (`player_effects_obj_opus5_150k_w1_20260914.md`) recorded five dead shapes: `&= ~FLAG(bit)`,
`&= (unsigned)~FLAG(bit)`, a function-scope `effect`, `char flags`, `long flags`. This wave re-measured the
spelling family and confirmed it is inert: `sfA` (`effect->flags &= ~FLAG(bit)`), `sfB` (`&= (byte)~FLAG(bit)`)
and `sfC` (explicit `flags = flags & (unsigned)~FLAG(bit)`) all reproduce the 816-byte split form. The macro,
the mask width and the assignment form are **not** the lever.

### Field width is not the lever either (control)

Editing the TU-local `struct player_effect_datum` field:

| `flags` type | this site | `_player_effect_update_screen_flash` | `_player_effect_update_camera_shake` |
| --- | --- | --- | --- |
| `byte` (as written) | split, 816 | EXACT 240 (January's register split) | EXACT 208 (January's register split) |
| `word` | in place, 800 [sha] | 256 (`or byte ptr [x],1`) | 192 (`or byte ptr [x],4`) |
| `unsigned long` | split, 816 | 240 [sha] | 208 [sha] |

January itself emits the register split form at those two sites
(`mov al,[ebx+0xe8]; … or al,4; … mov [ebx+0xe8],al`), so the field really is a `byte` and the in-place form at
`get_screen_flash` is **context**-driven.

### The mechanism (new measured law, proposed A46)

Two diagnostics isolated it:

* `sfD` — the clear moved after `screen_flash->type = render_screen_flash_type_map[effect->screen_flash.type];`
  → in-place `and`, 219 instructions, 800 padded (only the block order is then wrong).
* `sfF` — the clear followed by the `screen_flash->color` struct copy → in-place `and` **at January's own
  offset**.

So the form is selected by the *following* statement: when the next statement starts with a hoistable scalar
load through the **same base pointer** (`movsx edx,[edi+0x18]`), VC7's scheduler hoists that load between the
flag load and the `and`, and the memory RMW is not formed; when it does not, the single in-place RMW survives.

**Law A46 (refines the accessor-binding law / laws_w2 A5).** For a `byte` flag member, VC7 emits the single
in-place `and/or byte ptr [mem],imm` only if the next statement does not begin with a hoistable scalar load
through the same base pointer. Binding the read sub-struct to its own pointer local blocks the hoist and
restores the in-place RMW. The byte form therefore *reads out* whether January bound the sub-struct at that
site — January uses both spellings, exactly as the accessor-binding law says.

### The landed source

```c
struct screen_flash_definition const *flash = &effect->screen_flash;

SET_FLAG(
    effect->flags,
    _player_effect_screen_flash_just_started_bit,
    FALSE);

screen_flash->type = render_screen_flash_type_map[flash->type];
screen_flash->color = flash->screen_flash_color;
if (flash->duration > 0.0f)
    screen_flash->intensity = transition_function_evaluate(
        flash->fade_function,
        flash->zero_scale_factor * ((real)effect->screen_flash_time_left / flash->duration));
else
    screen_flash->intensity = flash->zero_scale_factor;
```

Authenticity: the block reads five members of one sub-struct (`type`, `screen_flash_color`, `duration`,
`fade_function`, `zero_scale_factor`), so a bound pointer is the natural spelling and satisfies the laws_w2 A5
counter-law (a pointer local with four or more uses is genuine); it is not the R16 device (it names real storage
in the datum, not a same-block local, and every member is still read by name through it). The name `flash` is
needed because this function's own output parameter already occupies the name `screen_flash`; the `const`
qualifier follows the TU's convention for read-only definition pointers
(`struct screen_flash_definition const *screen_flash` in `player_effect_update_screen_flash`). The `const`
variant was measured and is also strict EXACT.

**Independent confirmation of A46 inside the same object:** in `_player_effect_get_camera_effect_matrix`
January emits the *split* RMW at `+0x2a4`, and the next instruction is `fld [ebx+4]` — the `effect->direction`
read of the inlined cross product, through the effect pointer. Binding that vector (probe `cm1`) turns our RMW
into the in-place form, i.e. the two sites' byte forms are consistent and opposite, and each one tells you
whether January bound the sub-struct there.

## Rows that did not land

| Function | Padded | Classification | Shapes this wave |
| --- | ---: | --- | --- |
| `_player_effect_add_continuous_effect` | 320 | x87 evaluation-order tie (2 transposed ops) | acB (sub-struct binding), acC (weight-first product) — both inert |
| `_player_effect_get_camera_effect_matrix` (parked) | 1312 | m1/cross-product coupling + one x87 forwarding | cm1 (direction binding, 1296 and refuted by A46), cm2 (camera_impulse binding, 1312 [sha], worse) |
| `_player_effect_start` (parked) | 736 | A35(1) offset-0 leaf-age tie (2 instructions) | 0 — no admissible untried shape |
| `_player_effect_update_camera_impulse` (parked) | 752 | x87 term-order tie in inlined helpers | 0 — SKIPPED-EXHAUSTED |

Details, byte evidence and reopen criteria per function are in `scratch/workers/w3c_player_effects.md`.

### `_player_effect_add_continuous_effect`
January evaluates `(1.0f - weight)` before `periodic * weight` (`fld 1.0; fsub; fxch st(1); fmul; faddp`) even
though the call result is already in st(0); ours consumes the call result first. Both leaves are the same memory
operand `[esi+0x60]`, so A35 leaf age cannot discriminate, and no other law in laws_w2/laws_w3 fires. Five
shapes are now spent across w1 and w3c. Reopen only for evidence of a different expression tree, or a measured
rule for which operand of a `+` VC7 evaluates first when the other is already on the x87 stack.

### `_player_effect_get_camera_effect_matrix` (parked)
The w1 scratch body `pe_m6.c` re-gates at 1312/49 with four differing instructions: two operand loads in the
inlined `cross_product3d(global_up3d, &effect->direction, &axis)` (A35(1) offset-0/global-first quirk, flipped
by the `set_real_point3d` expansion in the *scripted* branch — an exclusive-or with the scripted block), and one
x87 value (`translation * direction.i`) that January keeps in a register across the y/z stores while ours spills
it. Not exact, so no reopen is proposed (and an R11 size-changing re-baseline is not proposable). The park is
untouched.

### `_player_effect_start` (parked)
One transposed pair: the offset-0 term of the inlined `dot_product3d(&left, &delta)`. `left` is the younger leaf
(written by the inlined `cross_product3d` after `delta`), January loads the younger leaf in all three terms and
our build loads the older one for the i term only — the A35(1) anomaly verbatim. The producer order is fixed by
January's own emission order, so the levers A35 offers would break byte-identical regions; A35 is diagnostic
only and its measured closures all used forbidden forms (R15 hand expansion, or the `_point_from_line3d`
COMDAT which is OUT in this lane).

### `_player_effect_update_camera_impulse` (parked)
Three x87 operand/term-order clusters inside inlined helpers, no structural delta. Every applicable law (A24,
A6, A31, A35, A46) was run as a detector and none yields an admissible untried shape.

## Proposals

No header, config or symbols.json change is needed or proposed. No park reopen is proposed. The one new law
(A46) is reported for the lane law file; it needs no tree change.

## Verification performed

- `python -B tools/campaign/gate.py source/effects/player_effects --all --forbid-emitted-symbol _point_from_line3d`
  before and after, compared row by row: only the `_player_effect_get_screen_flash` row changed.
- `python -B scratch/parkcheck.py source/effects/player_effects <candidate.obj>` → `parks 3 drift 0`.
- owner census against `build/base/source/effects/player_effects.obj` and
  `build/split/source/effects/player_effects.obj`: no candidate-only owner, no lost owner.
- `python -B tools/fake_match_scan.py source/effects/player_effects.c` → 0 review leads.
- No ninja, configure, pytest or git state command was run; no header, config or other source file was touched.
