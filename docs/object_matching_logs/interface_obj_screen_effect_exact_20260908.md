# `interface.obj` screen-effect caller recovery (2026-09-08)

## Result

This packet reconstructs `interface_draw_screen` in ordinary typed C from the
January PC COFF and authenticated HCEA source.  The retained body is strict
exact under the PID-isolated campaign gate:

| function | meaningful | padded | relocations | normalized SHA-256 | result |
| --- | ---: | ---: | ---: | --- | --- |
| `_interface_draw_screen` | 917 | 928 | 49 | `f8dedcfa84a428723351a804aecf08f6e16608d79891417ef730b85295cf30b8` | **strict exact** |

The unit advances from 14 to 15 strict-exact functions and from 2,068 to
2,985 meaningful exact code bytes.  Padded exact credit advances by 928 bytes.
All 14 inherited exact sections remain strict.  The three remaining unwritten
sections total 2,992 padded bytes:

- `_code_000ce320` / semantic `interface_get_weapon_hud_index`: 448 bytes;
- `_code_000ce900` / semantic `render_debug_profile_stall_tick`: 272 bytes;
- `_code_000cea10` / semantic `render_debug_profile`: 2,272 bytes.

The 272-byte profile helper is not an independent leaf: January gives it a
private register-argument convention chosen together with its sole 2,272-byte
caller.  This packet therefore does not manufacture a standalone signature or
claim fuzzy credit for either profiler section.  The previously investigated
weapon-HUD resolver remains unwritten rather than reintroducing its known
`fld`/`fstp` residual.

## January and source evidence

January locates the function at `0x000CE4E0`, with 917 meaningful bytes and a
928-byte padded COMDAT.  Its target section has external storage class 2,
function type `0x20`, 294 decoded instructions, and 49 relocations.  The final
candidate has the same instruction stream, padded bytes, relocation addresses,
types, targets, addends, and normalized digest.

The primary readable donor is:

`research-cache/halocea-full-blobs-20260830/src/blam/interface/interface_draw_screen.c`

Its file SHA-256 is
`180ca7b6943a6851a54982cfe6175eaaadf270a373667adb76ec3d938821dda0`.
The associated database-derived HCEA headers authenticate the complete
`weapon_hud_interface_definition`, the 0xB8-byte
`hud_screen_effect_definition`, the flag bit names, and the 0x38-byte
screen-effect parameter record.  January PC COFF, rather than donor prose,
governs source order and exact acceptance.

The body performs the following complete operation:

1. return immediately when the render window has no local player;
2. resolve the active weapon HUD and flashlight power;
3. fetch the first weapon-HUD screen-effect definition;
4. zero a 0x38-byte cinematic screen-effect parameter record;
5. select the fullscreen or splitscreen convolution mask;
6. compute the clamped field-of-view interpolation and warp radius;
7. apply flashlight and script scaling to light enhancement and desaturation;
8. copy the additive flag and desaturation tint;
9. submit either the populated parameter record or `NULL` to the rasterizer;
10. draw the HUD and run the game-engine post-rasterize stage.

The source uses project `real`, `word`, `boolean`, `tag_reference`,
`tag_block`, `bitmap_group_get`, `TAG_BLOCK_GET_ELEMENT`, `TEST_FLAG`, and
`PIN` facilities.  The full layouts and the offsets actually consumed by
January are compile-time checked.  There are no address-coded identifiers,
manual flag masks, raw field dereferences, representation reads, optimizer
directives, forced retention, or private helper emission.

## Exact source-order closure

The first complete candidate already had the exact 928-byte size, 49
relocations, 294 instructions, frame size, register assignment, and every
instruction outside one coherent conditional region.  HCEA rendered the
convolution-radius equality arm first:

```c
if (in_bounds[0] == in_bounds[1])
    radius = out_bounds[1];
else
    scalars_interpolate(...);
```

January branches to the equality fallback after the interpolation arm.  The
authenticated equivalent source order is therefore:

```c
if (in_bounds[0] != in_bounds[1])
    scalars_interpolate(...);
else
    radius = out_bounds[1];
```

Restoring that January order closes the entire section.  No expression sweep,
compiler flag change, artificial lifetime, or byte-forcing construct was used.

The repeated calls to `rasterizer_script_screen_effect_get_value` are not a
decompiler artifact.  They arise naturally from the project's `PIN` macro,
which evaluates a nontrivial argument in its lower comparison, upper
comparison, and selected-value arm.  January has exactly the corresponding
three-call schedule for each of the light-enhancement and desaturation script
sources.

## Caller and owner integration

January `render.obj` has one external caller: private `_code_001746f0` carries
an i386 REL32 relocation to `_interface_draw_screen` at section offset
`+0x392`; current readable source calls it from `source/render/render.c:427`.
The public declaration therefore belongs in `source/interface/interface.h` and
is added by this packet.  The temporary declaration formerly carried by
`source/render/render.c` has been removed.

The exact caller's relocation at `+0x19` names January placeholder
`_code_000ce320`.  HCEA at `0x836A6630` and the January symbol atlas both
authenticate the callee as `interface_get_weapon_hud_index` with signature:

```c
long interface_get_weapon_hud_index(real *flashlight_power);
```

Source retains that semantic name and the reviewed target symbol correction at
`config/symbols.json` file offset 844576 (`0x000CE320`) now owns the same name.
The final strict gate therefore requires no command-line alias.  The resolver
itself remains unwritten and receives no credit.

The pre-existing rasterizer owner mismatch is also corrected.  January and
HCEA prove that public `rasterizer_screen_effect` accepts a pointer to the
0x38-byte cinematic parameter record, not the unrelated 8-byte per-window
render record or the 0x78-byte storage-state wrapper.  The parameter type now
lives in `rasterizer_cinematics.h`; the public wrapper, Xbox implementation,
and cinematic-parameter resolver share the same mutable parameter-pointer
contract.  Mutation is intentional: the resolver may return the global record
that the Xbox implementation consumes and updates.  There is no caller-local
prototype, representation cast, or temporary integration scaffold left.

## Verification

- Baseline snapshot:
  `scratch/batch-10k-baseline-4ebe0ff6.json` — interface 14 exact, zero
  residual, four unwritten, 3,920 padded bytes unwritten.
- Final isolated command:
  `python -B tools/campaign/gate.py source/interface/interface --all --forbid-emitted-symbol _point_from_line3d`.
- Final result: 15 exact, zero residual, three unwritten.
- `_point_from_line3d` emitted-symbol guard: pass.
- Candidate object SHA-256:
  `8215fdbe1cca0d7a7eb87a637fbbfd91bc4752509805ffeec81bd0f8f989bbd`.
- Current January split-object SHA-256:
  `2464c7c3ff0f86222d7f6dda47b3e42c1e176f2be036d6c890412af2ea5871c9`.
- `fake_match_scan.py source/interface/interface.c`: zero review leads.
- `git diff --check` for the two owned source paths: pass.

No configure, Ninja build, staging, commit, push, shared config edit, parked
ledger edit, or unrelated source change was performed by this packet.
