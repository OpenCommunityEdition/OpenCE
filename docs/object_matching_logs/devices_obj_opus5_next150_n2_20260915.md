# `devices.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

## Scope and provenance

- Translation unit `source/devices/devices.c`. Target `build/split/source/devices/devices.obj`
  (January 2002 Xbox debug build; XDK 3911 `CL.Exe` 13.00.9254).
- Lane root `C:\halo-worktrees\opus5-150k-house-clean-20260914`, HEAD `3083f800b`, worker
  `n2:collision_usage+devices`. Only `source/devices/devices.c` was modified. No header, `config/*.json`,
  `symbols.json`, parks, build or tooling change. Scratch: `scratch/workers/n2_collision_usage_devices/`,
  notes `scratch/workers/n2_collision_usage_devices.md`.
- Read first: `scratch/opus5-next150/WORKER_BRIEF.md`, `scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`, and
  the three earlier devices ledgers (`devices_obj_jonas_position_setters_wave_20260821.md`,
  `devices_obj_frontfacing_withheld_owner_boundary_20260905.md`,
  `devices_game_sound_small_fuzzy_harvest_20260908.md`).

## Result

| gate (`--forbid-emitted-symbol _point_from_line3d`, guard passed) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (HEAD) | 29 | 1 | 0 |
| final (real file) | **30** | **0** | **0** |

`devices.obj` is now closed: every listed function is strict EXACT.

| function | target padded / meaningful / relocs | before | after |
| --- | --- | --- | --- |
| `_device_frontfacing` | 128 / 119 / 4 | 128 / 4, `[sha]` (fuzzy, zero credit since 2026-09-08) | **strict EXACT** |

Newly exact this wave: `_device_frontfacing`, **128 padded bytes**.

## `_device_frontfacing`

### Structural census (run before any shape)

Frame: January `sub esp,0x6c`; ours `sub esp,0x6c` — identical. Slots present on both sides and nowhere else:
`[ebp+8]` `device_index`, `[ebp+0x10]` `facing`, `[ebp-0x6c]` the 0x6c-byte `object_marker front_marker`,
and `[ebp-0x30]/[ebp-0x2c]/[ebp-0x28]` = `front_marker.matrix.forward.i/.j/.k`. Ours adds no slot and
lacks none.

Relocations by target: 4 vs 4, same four destinations at the same four addresses —
`_object_try_and_get_and_verify_type` @0x12, `??_C@_05HJNNLMDJ@front?$AA@` @0x30,
`_object_get_marker_by_name` @0x36, `__real@00000000` @0x5c. No call and no global is missing or added.

The row was therefore not structural: 58 target instructions, 58 ours, 56 aligned equal.

### The single hunk

`python -B -m tools.campaign.alndiff source/devices/devices _device_frontfacing` reported one transposition:

```
January:  fld [ebp-0x2c] ; fmul [eax+4]      (j term)
          fld [ebp-0x28] ; fmul [eax+8]      (k term)
ours:     fld [ebp-0x28] ; fmul [eax+8]      (k term)
          fld [ebp-0x2c] ; fmul [eax+4]      (j term)
common:   faddp st(1) ; fld [ebp-0x30] ; fmul [eax] ; faddp st(1)
          fcomp __real@00000000 ; fnstsw ax ; test ah,0x41 ; jne
```

Both sides fld the young leaf (the `front_marker` local, written by the preceding call) and fmul the old one
(`facing`, a parameter), and both associate `(a + b) + i`. Only the order of the two independent products
differed.

### Source defect and fix

The body in production since 2026-09-08 spelled the dot product as three hand-written component products:

```c
real facing_dot_front =
    facing->i * front_marker.matrix.forward.i +
    facing->j * front_marker.matrix.forward.j +
    facing->k * front_marker.matrix.forward.k;
```

That is a hand-expanded helper body — the class prohibited by house rule 13 and laws_w3 R15, and the thing
the 2026-09-05 ledger itself asked not to do ("do not replace the existing math call with a hand-expanded
expression"). The project's genuine helper is `source/math/real_math.h:1209`:

```c
__inline real dot_product3d(real_vector3d const *a, real_vector3d const *b)
{ return a->i*b->i + a->j*b->j + a->k*b->k; }
```

Restoring the call (laws_w2 A4, HELPER ROUTING: inlined helper parameters are evaluated together, so the
association is uniform across rows) reproduces January exactly.

| shape | change | result |
| --- | --- | --- |
| v1 | `real facing_dot_front = dot_product3d(facing, &front_marker.matrix.forward);` | **strict EXACT**, 128/4; whole TU 30 exact / 0 residual / 0 unwritten. **Landed.** |
| v2 | `dot_product3d(&front_marker.matrix.forward, facing)` | also strict EXACT; not chosen |

v1 is the natural spelling: the argument order is the one the result name states (`facing_dot_front` =
facing · front). Both orders being exact is itself the proof that this landing is not an operand permutation
used to steer x87 (laws_w2 R3) — the helper call removes the hand-expansion, and the term order follows from
leaf age, not from how the source was written.

Semantics, unchanged from the reviewed 2026-09-05/09-08 packet: a checked `object_mask_control` lookup, the
`_control_usable_from_both_sides_bit` flag test, exactly one `"front"` `object_marker`, and
`!(dot > 0.0f)` — the polarity that keeps January's NaN/unordered result (`test ah,0x41`), which the HCEA
mirror's `<= 0.0f` does not.

### Owner census and the 2026-09-05 ownership veto

Versus `build/base/source/devices/devices.obj` the candidate adds exactly one code owner, `_dot_product3d`
(`.text`, external, 32 bytes, COMDAT selection 2 = `IMAGE_COMDAT_SELECT_ANY`), plus a compiler-local label
renumber `$L7670` -> `$L7672`. No new data, BSS or COMMON owner; `"front"`, `__real@00000000`,
`__real@3ecccccd`, `__real@3f800000` and `??_C@_00CNPNBAHC@?$AA@` were already present and are inherited.
The emitted `_dot_product3d` is 32 bytes, sha16 `d564ef69fd99449f` — **byte-identical to January's own
selected copy** in `build/split/source/ai/action_charge.obj`.

`devices_obj_frontfacing_withheld_owner_boundary_20260905.md` withheld the helper-call form because that
select-any COMDAT raises `LNK2005` against `action_charge.obj`'s selection-1 (`SELECT_NODUPLICATES`) copy.
Measured across the whole tree in this wave (COMDAT selection read from each definition's section aux
record):

- `build/base`: **70** objects define `_dot_product3d` — **69 at selection 2**, and only
  `build/base/source/ai/action_charge.obj` at selection 1. All 70 copies have identical bytes.
- `build/split` (January): exactly **one** definition, `action_charge.obj`, selection 1.

The accepted board therefore already carries 69 select-any surplus copies beside the no-duplicates owner;
this landing makes it 70, with byte-identical contents. The `LNK2005` the veto demonstrated is a property of
the accepted tree as a whole, not something this row introduces. The current binding rules name this owner
as accepted: laws_w2 A4 census ("real_math.h select-any surplus COMDATs (`_dot_product3d`,
`_set_real_point3d`, `_sine` and similar) are accepted as systemic; `_point_from_line3d` and
`_vector_from_points3d` are not") and WORKER_BRIEF section 6 item 5. The brief's FINAL owner rulings list
(section 1) does not contain this veto and keeps only `_point_from_line3d` out of this object.

The landing is made on that basis and is flagged here for the verifier. If laws_w3 R13 is read as binding
the 2026-09-05 veto regardless of the census, the revert is one hunk: restore the three component products
recorded in `scratch/workers/n2_collision_usage_devices/base_body.txt`; the row returns to `[sha]` at 128/4
and the object to 29/1/0.

### Checks

- Strict EXACT in a whole-TU gate of the complete candidate file; no alias, no `--cflag`.
- Sibling rows: `diff` of the full `--all` listings before/after changes exactly two lines (the target row
  and the summary). All 29 baseline-exact rows stay exact.
- `_point_from_line3d` guard: passed.
- Parks: `scratch/parkcheck.py source/devices/devices` reports "no parks for source/devices/devices"; no
  park drift is possible for this unit.
- Owner census: as above, inside the accepted systemic select-any class.
- `tools/fake_match_scan.py source/devices/devices.c`: 0 review leads.
- Intent: the original programmer asked "does this direction point into the operable face of the device?" —
  a checked control lookup, a both-sides-usable early out, the device's `"front"` marker, and the project's
  own `dot_product3d` against that marker's forward axis, written so that a NaN or non-positive dot answers
  "front facing".
- `branch_sweep.py source/devices/devices`: 23 historical blobs, best 29/0/1 (frontfacing unwritten); no
  historical blob reached this state.

## Not spent

Nothing else in this object: every other row was strict EXACT at baseline and is untouched.
