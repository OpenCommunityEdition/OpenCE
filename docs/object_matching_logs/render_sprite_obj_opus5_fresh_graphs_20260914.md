# `render_sprite.obj` Opus5 fresh-graphs wave 2 (20260914)

## Scope and provenance

- Translation unit `source/render/render_sprite.c`; January target `build/split/source/render/render_sprite.obj`.
- Lane `opus/30k-fresh-graphs-20260914`, lane HEAD `423067414`; worker scope was this one `.c` file.
- Compiler: XDK 3911 Microsoft C/C++ 13.00.9254 with the repository flags (`/O2 /Oy- /DDEBUG /Dxbox`),
  measured through `tools/campaign/gate.py --source` whole-TU compiles.
- Evidence used: January COFF bytes and relocations (authoritative), January map atlas
  (`scratch/atlas/render_sprite.obj.txt`: all ten names), Ghidra decompilation
  (`scratch/ghidra/out/render_sprite.obj.decomp.c`), HCEA per-function reconstructions
  (`halocea-review/src/blam/networking/build_sprite*.c`, semantics only), prior ledgers
  `render_sprite_obj.md`, `render_sprite_obj_jonas_public_salvage_20260821.md`,
  `opus_misc_small_admission_20260903.md`, and the donor branch `opus/small-families-30k-20260902`.

## Validated baseline

Lane HEAD gate: `== exact 6  residual 4  unwritten 0  (of 10 listed)`, `_point_from_line3d` guard passed.

| Function | Padded | Relocs | Baseline candidate | Target sha16 | Tag |
| --- | ---: | ---: | --- | --- | --- |
| `_build_sprite` | 1552 | 59 | 1536 / 59 `7a46776a809c0105` | `2a5e72c78e39ebdd` | size, sha |
| `_build_sprite_rotational` | 640 | 38 | 656 / 38 `7341abbbb3df13ec` | `99c5dce7c5488e3c` | size, sha |
| `_build_sprite_get_group` | 384 | 20 | 384 / 20 `406061bd60540993` | `f2e9787070fc78bf` | sha |
| `_build_sprite_transform_origin_and_direction` | 304 | 27 | 304 / 27 `ec336afdd407dcfc` | `c6b9005a5dc28248` | sha |

The other six functions (`compute_basis`, `compute_scale`, `compute_vertex_fade`, `prepare_for_window`,
`build_sprites_begin`, `build_sprites_end`) were exact at baseline and stay exact. No `config/parked.json`
entry exists for this unit.

## Accepted controls (landed)

### `_build_sprite_get_group` -> strict EXACT (384 / 20, sha `f2e9787070fc78bf`)

First divergences at baseline were two instructions only: the full-table early exit
(`mov eax,0xffffffff` in January, `or ax,0xffff` for us) and the final return (`mov ax,di` in January,
`movsx eax,di` from our int-promoted ternary). January's control flow also re-tests
`group_index < group_count` after the `group_count < 8` test, which is exactly an `if` that repeats the
vassert condition with the allocation nested inside. The landed body therefore:

- guards creation with `if (group_index<data->group_count || data->group_count<MAXIMUM_BUILD_SPRITE_GROUPS)`
  (the assertion's own condition), with `else group_index= NONE;`;
- invalidates a group without locked vertices with `group_index= NONE;` and returns `group_index` once
  at the bottom (single-exit law). The copy-propagated `mov eax,-1` is the else-arm NONE threaded into
  that return; the `or eax,-1` is the invalidation store; `mov ax,di` is the short-typed single return.

HCEA's reconstruction has the same nested full-table else-arm (`index = -1`) and final `index == -1` /
vertices test. Checks: per-function sha diff shows only this row changed; `_point_from_line3d` guard
passed; owner census (code and data owners vs `build/base`) unchanged; `fake_match_scan.py` 0 leads.

## Experiment matrix

| Target | Shape | Result | Kept |
| --- | --- | --- | --- |
| get_group | A: vassert-condition `if` + else NONE + single-exit invalidation store | 384/20 EXACT | landed |
| get_group | B: A with `return NONE` in the invalidation | 384/20 sha | no |
| get_group | C: A head with the old ternary return | 384/20 sha | no |
| transform | current named local output vector | 304/27 sha | baseline |
| transform | `(real_vector3d *)&transformed_origin` (original bug, see below) | 304/27 EXACT; rotational becomes 640 | no (UB policy) |
| rotational (on UB transform) | current body | 640/38, 26 diff blocks | scratch only |
| rotational (on UB transform) | r1: edge flags declared before the sequence lookup | 24 blocks; flag store placement matches | scratch only |
| rotational (on UB transform) | r1p: r1 + float-literal quarter-circle constant (probe) | constant matches; only angle frame slot differs | probe only, not authentic |
| rotational (on UB transform) | r2: function-scope flags local | 26 blocks | no |
| rotational (on UB transform) | r3: angle declared before fraction | no frame change | no |
| rotational constant | `(_pi/2)*(_pi/2)`, `(_pi/2.f)*(_pi/2.f)`, `_pi*_pi/4.f`, `(real)` casts, `_pi/2*_pi/2`, `_pi*0.5f` | all fold to `__real@3ecf817b` | no |
| build_sprite (on UB transform) | current body | 1536 vs 1552, 517/517 insns, 95 blocks | scratch only |
| build_sprite (on UB transform) | bs1: `registration_point.x + bounds.x0`, rotated y before x (HCEA order) | 1536, 77 blocks, worse schedule | no |
| build_sprite (on UB transform) | bs2: bs1 + debug vertex pointer locals | object identical to bs1 | no |

All scratch sources/objects: `scratch/workers/render_sprite/`.

## Residual classification

### `_build_sprite_transform_origin_and_direction`: original-bug source, owner-policy blocked

Measured fact: the January private ABI is ebx=data, esi=untransformed_origin, edi=untransformed_direction,
stack `[ebp+8]` flags, `[ebp+0xc]` transformed_origin, `[ebp+0x10]` transformed_direction (proved from
the rotational caller's pushes). The screen-space arm has no frame (`sub esp` absent) and passes
`lea eax,[ebp+0xc]` - the address of the `transformed_origin` parameter home - as
`render_camera_screen_to_view`'s 12-byte output. Only `(real_vector3d *)&transformed_origin` reproduces it
(strict exact). Inference: an original Bungie typo (`&transformed_origin` for `transformed_origin`), latent
because screen-space sprites are otherwise unsupported ("tell Bernie"); HCEA independently describes the
PPC arm as writing to "a throwaway stack scratch buffer" and discarding it. The spelling writes 12 bytes over
two 4-byte parameter homes plus 4 bytes of caller frame, so it is undefined behaviour under the lane rules and
was already rejected once (`opus_misc_small_admission_20260903.md`). It is therefore not landed and is filed
as an owner-policy proposal. Because the legal body enregisters `transformed_origin`, it changes the private
ABI seen by both callers.

### `_build_sprite_rotational`: ABI-coupled, then constant precision + frame packing

With the UB transform body it is already 640/38 with 211/211 instructions. Remaining facts: January's
`__real@3ecf817a` is 1/((pi/2)^2) folded from float-rounded pi, while every `_pi` spelling folds in extended
precision to `...7b` (float literals give `...7a`); January keeps `angle` in a frame slot at `[ebp-8]` with
holes at `-0xc`/`-4`, whereas VC7 packs all three scalars into parameter homes for us (frame 0x18 vs 0x24).

### `_build_sprite`: ABI-coupled, then broad register/x87 residual

Even on the UB base it is 16 bytes short: frame 0xb0 vs 0xac (one extra 4-byte slot), offset_y spilled to
color's parameter home with `sine*offset_x` computed before x, mirror flags enregistered while `data` is
reloaded from its home, debug tail hoists three vertex pointers into callee-saved registers and reuses the loop
`point` slot. The `bounds.x0 + registration_point.x` load order does not follow source operand order in VC7,
so January has a different node shape there. Unresolved; not a single tie.

## Do-not-repeat list

- get_group: done (exact). Prior negatives (short/unsigned-short return type on the old early-return shape,
  named short result with long prototype) remain valid for those shapes.
- transform: no legal spelling produces `lea [ebp+0xc]` without a frame; do not sweep local names/scopes.
- rotational: `_pi` constant spellings (six measured), declaration-order and function-scope flag variants,
  and all family-2 shapes from `render_sprite_obj.md` under the legal transform ABI.
- build_sprite: HCEA operand/rotation order, debug vertex pointer locals (folds to identical object), a hoisted
  `shader` local (contradicted: January reloads `data->shader` for the blend test).
- Prohibited: UB spellings in production without an owner ruling; float literals replacing `_pi` without
  January-authentic provenance; any shared-header edit (real_math.h is declaration-count sensitive).

## Reopen criteria

1. Owner ruling admitting the proven original-bug spelling for the transform screen-space arm (unlocks
   transform immediately and restores the January ABI for both callers).
2. For rotational: a January-authentic float-precision quarter-circle/half-pi constant (e.g. a cseries/real_math
   constant evidenced in another exact object), then the angle frame-slot packing (best body: r1).
3. For build_sprite: a source recipe predicting the extra frame slot and the loop's offset_y memory home.

## Disposition

`render_sprite.obj` advances from 6/10 to **7/10 strict exact** (+384 padded bytes, `_build_sprite_get_group`).
Three residuals remain active, all downstream of the transform original-bug ruling. The object is not
`Matching`.
