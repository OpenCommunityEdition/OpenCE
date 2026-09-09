# interface.obj — 6/18 to 14/18 exact, and `.data` fully recovered (Opus lane, 2026-09-06)

Lane `opus/30k-ui-script-profiles-20260906`, base commit `19765bb76`.
Compiler XDK 3911 VC7, unchanged repository production flags. No flag
overrides, no inline assembly, no pragmas.

## Disposition

Eight functions newly exact (1,888 padded code bytes):

```
 96  _interface_initialize_for_new_map
128  _interface_get_real_argb_color
128  _interface_get_tag_index
128  _profile_graph_toggle
224  _interface_splitscreen_render      (was _code_000cf2f0)
352  _interface_draw_bitmap_modulated_p32
384  _interface_draw_bitmap
448  _interface_draw_bitmap_modulated
```

**The whole `0x18918`-byte `.data` section is now byte-exact** — 100,632 bytes
with 18 relocations, `section_infos_equal` True. The three `profile_*_values`
tables were recovered directly from January's object bytes; `interface.obj` owns
them as file-local statics.

Object gate: `exact 14  residual 1  unwritten 3  (of 18 listed)`.

## Levers that did the work

* **`interface_tag_references_get()` must be a macro, not a function.** Both
  users emit `call scenario_get_game_globals` *twice*. The block expression is
  textually duplicated at each use, so neither a local nor a static helper can
  reproduce it.
* **`color_index %= colors.count` on the `short` parameter** supplies January's
  `movsx edx,dx`. Those are exactly the three bytes the 2026-08-21 lane was
  missing — its third relocation landed at `+0x4B` against January's `+0x4E`.
  **This closes the only residual documented in
  `interface_obj_jonas_color_pair_20260821.md`.**
* **Deleting named scalar locals** in `_interface_draw_bitmap` — writing
  `(bitmap->width*u - bitmap->registration_point_x)*scale` inline — makes VC7's
  loop-invariant hoist emit the `fild`s in January's order, keep the correct
  pair live on the x87 stack, and produce `sub esp,0xF4` on its own. (Note this
  is the *opposite* direction from the usual "name the value" reflex: what
  matters is the lifetime, and here the named local lengthened it.)
* **`point->x + local_x*cos - local_y*sin`, with the point term first**, yields
  January's `fld/fmul/fadd/fld/fmul/fsubp`. The HCEA donor's
  `lx*cos - ly*sin + px` does not — and that donor topology is precisely what
  the p32 ledger already listed as do-not-repeat.
* **VC7 emits struct-field stores in source order, unreordered**; the four
  `1.0f` parameter stores are a single chained assignment.
* **An assert placed between the stores and the call is what prevents VC7
  cross-jumping two identical call tails** in `_interface_splitscreen_render`
  (shapes 1-3 all tail-merged without it).

## Residual

`_interface_get_weapon_hud_index` (448 padded, was `_code_000ce320`) is at
**exact size and exact relocations**, 144 versus 145 instructions. The only
divergence: January copies the `real` out-parameter with
`mov edx,[ebp-4]` / `mov [eax],edx` where we emit `fld`/`fstp`.

Two candidate explanations were tested and **both falsified**:

* not register pressure — a GPR is free at both exits;
* not the `/Op` flag — gating the unit with `--cflag /Op` reproduces all 14
  exact functions and leaves this residual unchanged.

Parked. Reopen only with a legal-C control that makes VC7 move a `real` through
a general-purpose register rather than the x87 stack.

## Remaining unwritten

`_render_debug_profile` (2,272), `_interface_draw_screen` (928),
`_render_debug_profile_stall_tick` (272).

`_render_debug_profile_stall_tick` is a **static with a VC7 custom calling
convention** (`dx` / `eax` / `ecx` as register arguments) and is only
reproducible together with its caller — do not attempt it standalone.

## Recovered names

All four placeholders in this object are attested by the January symbol atlas
(`atlas_off = january_offset + 0x400000`, `lib` field confirming
`interface.obj`):

| placeholder | recovered name | atlas tier |
|---|---|---|
| `_code_000ce320` | `interface_get_weapon_hud_index` | exact (also HCEA `@0x836A6630`) |
| `_code_000cf2f0` | `interface_splitscreen_render` | exact |
| `_code_000cea10` | `render_debug_profile` | fuzzy |
| `_code_000ce900` | `render_debug_profile_stall_tick` | fuzzy |

The last two are marked fuzzy in the atlas but are mutually corroborating: they
form a coherent debug-profiler cluster with the now-exact `_profile_graph_toggle`,
which toggles exactly that display.

## Environment trap recorded for other workers

Writing this file with a bash heredoc **silently eats one level of
backslashes**, mangling `match_assert` path literals into a different `.rdata`
symbol. Use an editor tool for any line containing a path literal. (This matches
the warning already in `tools/campaign/README.md` about `apply_edits.py`.)

---

The remainder of this document is the per-function working record produced
during the wave, preserved verbatim.

---

# worker `interface` — `source/interface/interface.c`

base gate (reproduced): `== exact 6  residual 0  unwritten 12  (of 18 listed)`
Never end below 6 exact.

## target facts recovered up front

* `scenario_get_game_globals()->interface_tag_references` is at `+0x140`
  (game_globals.h: flags + unused0[61] + 7 tag_blocks = 0x140). Element size
  `0x130`.
* Both `_interface_get_tag_index` and `_interface_initialize_for_new_map`
  emit `call scenario_get_game_globals / mov ecx,[eax+0x140] / test ecx,ecx /
  je NULL-arm` and then **call `scenario_get_game_globals` a second time**
  inside the taken arm. The block expression is therefore textually
  duplicated => a file-local *macro*, not a local variable and not a static
  helper.
* The `0x130` element is `struct tag_reference tags[16]` (`.index` at +0xC,
  stride 16 -> `mov eax,[ecx+eax+0xc]` with `ecx = index<<4`) followed by
  0x30 unused bytes.
* assert line anchors in this object (usable with `match_assert`):
  - `_interface_get_tag_index` line **109**, text
    `interface_tag_index>=0 && interface_tag_index<NUMBER_OF_INTERFACE_TAGS`
  - `_code_000cea10` line **704**, text `drawingbuf_counts[index] < 512`
  - `_code_000cf2f0` line **884**, text `window_count==4`
  (=> the original interface.c was ~950 lines; natural `assert()` can never
  reproduce these, `match_assert` is mandatory.)
* colour table tag is `'colo'`; element size `0x30`
  (`char name[32]` + `real_argb_color color` at `+0x20`), block at `+0x00` of
  the tag.

### _interface_get_tag_index  (padded 128 bytes)
status: EXACT
gate line: `EXACT       128  _interface_get_tag_index`
shapes tried: 1 -- `match_assert(file,109,expr)` then
  `return interface_tag_references_get()->tags[interface_tag_index].index;`
  where the accessor is the file-local ternary macro described above. EXACT
  first compile.
evidence: January COMDAT at 0x000CE1D0; assert strings
  `??_C@_0EH@NDHILNFF@interface_tag_index?$DO?$DN0...` (0x47 = 70 chars) and
  `c:\halo\SOURCE\interface\interface.c`, `push 0x6d` = line 109,
  `push 1` = fatal. Tail-duplicated `movsx/shl 4/mov eax,[ecx+eax+0xc]` in
  both ternary arms proves the deref-of-ternary shape.
notes: the prior color-pair ledger (20260821) explicitly *excluded* this
  function calling its zero-count arm "raw/undefined behaviour". That reading
  was wrong: nothing in the source names address 0 -- the macro yields NULL and
  ordinary C then dereferences it, which is exactly what January does. No raw
  address, cast or pun is used.

### _interface_get_real_argb_color  (padded 128 bytes)
status: EXACT
gate line: `EXACT       128  _interface_get_real_argb_color`
shapes tried: 1 -- EXACT first compile. Key: `color_index %= color_table->colors.count;`
  on the **short** parameter. The truncate-back-to-short is January's
  `movsx edx,dx` at +0x48; the 20260821 first shot was missing exactly those
  3 bytes (its 3rd reloc landed at +0x4B instead of +0x4E).
  Second key: `color->alpha = color->red = color->green = color->blue = 1.0f;`
  -- the chained assignment gives January's descending +0xC/+8/+4/+0 store
  order with the constant hoisted into ecx. Four ascending `n[i]=1.0f`
  statements (the HCEA donor shape) would be the wrong order.
evidence: `push 0x636f6c6f` = 'colo'; block at +0 of the tag, element size
  0x30, colour at +0x20; HCEA `interface_get_real_argb_color.c` corroborates
  the modulo-wrap and the 48-byte `color_table_color` record.
notes: closes the one documented residual of the 20260821 wave.

### _interface_initialize_for_new_map  (padded 96 bytes)
status: EXACT
gate line: `EXACT        96  _interface_initialize_for_new_map`
shapes tried: 1 -- EXACT first compile.
evidence: three REL32 lifecycle calls, then the same guarded
  interface-tag-references fetch, then
  `draw_string_set_draw_mode(refs->tags[_interface_font_terminal].index, NONE,
  0, 0, global_real_argb_white)`. `[eax+0x1c]` = 16*1 + 0xC = slot 1 =
  `_interface_font_terminal`. `mov ecx,[global_real_argb_white]` is the
  *pointer* load (cseries.h declares it `const real_argb_color *`).
notes: none.

**unit after this wave: exact 9  residual 0  unwritten 9.**

### _profile_graph_toggle  (padded 128 bytes)
status: EXACT
gate line: `EXACT       128  _profile_graph_toggle`
shapes tried: 1 -- EXACT first compile.
evidence: signature `void profile_graph_toggle(char const *graph_name)` is
  already declared verbatim in `source/hs/hs.c:4341` (it is the HS script
  command `profile_graph_toggle`, and `hs.obj` is its only external caller).
  Loop over `profile_graph_values[0 .. profile_graph_value_count)`, match the
  name against BOTH strings, toggle the byte at +0x209.
notes: --

### BONUS: whole `.data` section recovered byte-exact
`interface.obj` owns a 0x18918-byte initialized `.data` section holding
`profile_game_value_count/_values`, `profile_frame_value_count/_values` and
`profile_graph_value_count/_values`. These are **file-local statics** (no other
January object references them) and they are **initialized**, not BSS, so the
table content is recoverable directly from January's object bytes.

Recovered layout, proven by `coff_compare.section_infos_equal` on the whole
section (size 0x18918, 18 relocations, normalized sha equal, relocations
equal -- `EQUAL True`):

```c
struct profile_value            /* 0x20C */
{
	char name[256];             /* +0x000  key for profile_find_frame_value */
	char label[256];            /* +0x100  short label drawn in the legend */
	real_argb_color const **color;  /* +0x200  &global_real_argb_<name> */
	short frame_value;          /* +0x204  filled at runtime, NONE in .data */
	short section_index;        /* +0x206  filled at runtime, NONE in .data */
	boolean subtract_previous;  /* +0x208  plot delta vs the previous series */
	boolean enabled;            /* +0x209  toggled by profile_graph_toggle */
};
```
`MAXIMUM_PROFILE_VALUES` is 64 (0x8300 / 0x20C). Counts are 3 / 1 / 14 and the
count `short` precedes each array (array lands at +8, MSVC 8-aligns it).
The colour field is a **pointer to pointer**: `global_real_argb_*` are
themselves `const real_argb_color *` variables, so only `&global_real_argb_x`
is a constant expression, and `_code_000cea10` does `mov ecx,[edi+0x200] /
mov edx,[ecx] / add edx,4` (deref, then skip alpha to get the `real_rgb_color`).

**unit after this wave: exact 10  residual 0  unwritten 8.**

### _interface_draw_bitmap  (padded 384 bytes)
status: EXACT
gate line: `EXACT       384  _interface_draw_bitmap`
shapes tried:
  1. explicit `real width/height/registration_x/... = (real)bitmap->width;`
     block before the loop, HCEA-style `lx*cos - ly*sin + px`, struct fields
     assigned in declaration order.
     -> residual [sha]: `sub esp,0xF0` vs January `0xF4`; `movsx` hoist order
     width/height/regx/regy vs January width/regx/height/regy; wrong pair
     left resident on the x87 stack; parameter stores in the wrong order.
  2. **deleted the six scalar locals** and wrote the conversions inline in the
     loop -- `(bitmap->width*u - bitmap->registration_point_x)*scale` etc.
     VC7's loop-invariant hoist then emits the `fild`s in expression order
     (width, registration_x, height, registration_y, point->x, point->y),
     keeps the FIRST TWO resident on the x87 stack, and the frame becomes
     `0xF4` on its own.  -> residual [sha], ONLY the 8 parameter stores.
  3. parameter stores reordered to January's -> **EXACT**.
evidence / the two levers:
  * **frame size is a local-count fact**: 0xF4 = vertices 0x50 + parameters
    0x8C + default clip 0x10 + exactly two 4-byte spills. Naming the six
    converted scalars forces six named locals and the wrong residency.
  * `x = point->x + local_x*cosine - local_y*sine` (point FIRST) is what
    produces January's `fld lx / fmul cos / fadd px / fld ly / fmul sin /
    fsubp`. The HCEA donor's `lx*cos - ly*sin + px` does NOT.
    `y = point->y + local_x*sine + local_y*cosine` is fine as written
    (VC7 reassociates the two commutative products).
  * VC7 emits the parameter stores in **source order, unreordered**. January's
    order is
    ```c
    csmemset(&parameters, 0, sizeof(parameters));
    parameters.map_scale[0].i = parameters.map_scale[0].j =
        parameters.map_texture_scale[0].i = parameters.map_texture_scale[0].j = 1.0f;
    parameters.meter_parameters = NULL;
    parameters.point_sampled = FALSE;
    parameters.framebuffer_blend_function = _shader_framebuffer_blend_function_alpha_multiply_add;
    parameters.map[0] = (struct bitmap_data *)bitmap;
    ```
    The four `1.0f` stores are ONE **chained assignment** (rightmost evaluated
    first => the emitted +0x44/+0x40/+0x2c/+0x28 descending order). This is the
    same author idiom as `color->alpha = color->red = ... = 1.0f` in
    `_interface_get_real_argb_color`.
  * loop bound is the signed enum `NUMBER_OF_POINTS_PER_RECTANGLE` (cseries.h),
    NOT `NUMBEROF(vertices)` -- January's `cmp si,4 / jl` is signed.
  * `#include "bitmaps/bitmap_group.h"` MUST come after `cseries/cseries.h`
    (it pulls real_math.h, which needs cseries types; alphabetical-first
    placement gives `C1046: union nested too deeply` in real_math.h).
notes: this closes the topology the 20260829 p32 ledger listed as
  do-not-repeat; see the p32 block below.

### _interface_draw_bitmap_modulated_p32  (padded 352 bytes)
status: EXACT
gate line: `EXACT       352  _interface_draw_bitmap_modulated_p32`
shapes tried: 1 -- EXACT first compile, `no normalized instruction differences`.
REOPENING the 20260829 do-not-repeat: that ledger's reopen criterion was "a
same-compiler natural C donor that independently fixes these exact
topologies". `_interface_draw_bitmap` (this session, EXACT) is exactly that
donor, and it fixes all four listed boundaries at once:
  * frame `0xF0` -- reached by NOT naming the converted scalars;
  * point X homed at `[ebp-4]` -- same consequence;
  * `local_x*cos + point_x - local_y*sin` -- from writing
    `point->x + local_x*cosine_theta - local_y*sine_theta`;
  * signed `JL` -- from `NUMBER_OF_POINTS_PER_RECTANGLE`, not `NUMBEROF()`;
  * blend store after meter+point_sampled -- the store order above.
signature: `(bitmap, point, clip, real scale, real theta, pixel32
  modulated_color, short shader_type)`; default clip is `{0, width, 0, height}`
  in **pixel** space, so `local_x = (u - registration_point_x)*scale` with no
  width factor.

### _interface_draw_bitmap_modulated  (padded 448 bytes)
status: EXACT
gate line: `EXACT       448  _interface_draw_bitmap_modulated`
shapes tried: 1 -- EXACT first compile.
evidence: identical to p32 except the modulation colour arrives as
  `real_argb_color const *` and is packed with the plain
  `(a<<24)|(r<<16)|(g<<8)|b` expression (VC7 turns that into the
  `shl esi,8 / or esi,eax` accumulate because `__ftol2` clobbers eax).
notes: --

**unit after this wave: exact 13  residual 0  unwritten 5.**
Remaining unwritten: `_code_000ce320` (448), `_code_000ce900` (272),
`_code_000cea10` (2272), `_code_000cf2f0` (224), `_interface_draw_screen` (928).

### _code_000cf2f0  (padded 224 bytes)  [rename proposal: interface_splitscreen_render]
status: EXACT
gate line: `EXACT       224  _code_000cf2f0`
shapes tried:
  1. common `bounds.x0/y1/x1` written once before the `==3` if, one shared
     `draw_quad` after the if/else -> residual [size 208!=224, relocs 9!=10]:
     the three common stores came out in FORWARD order and the two calls
     tail-merged.
  2. full field-order rectangle in BOTH arms, each with its own `draw_quad`
     -> the three common stores now hoist correctly (VC7 hoists them
     bottom-up, giving January's reversed x1/y1/x0), but VC7 still
     cross-jumped the two `call draw_quad / add esp,8 / epilogue` tails.
  3. same, rewritten with early `return`s (failure-first) -> byte-identical
     to 2; the CFG spelling is inert here.
  4. **assert moved to AFTER the rectangle stores, immediately before the
     call** -> EXACT, `no normalized instruction differences`.
evidence / lever: January puts `mov word ptr [ebp-8], 0` for the four-player
  arm *between* `cmp si,4` and `je` -- i.e. BEFORE the assert -- and does not
  cross-jump the two calls. Writing the arm as
  ```c
  bounds.y0 = 0; bounds.x0 = 319; bounds.y1 = 480; bounds.x1 = 321;
  match_assert("c:\halo\SOURCE\interface\interface.c", 884, window_count==4);
  draw_quad(&bounds, 0xFF000000);
  ```
  puts the assert between the stores and the call, which both places the
  `y0` store early AND blocks the tail merge. This is a reusable lever:
  **an assert sitting between the stores and the call is what stops VC7
  cross-jumping two otherwise identical call tails.**
  Geometry: 2 players -> {y0 239, x0 0, y1 241, x1 640} (horizontal bar);
  3 players -> {240, 319, 480, 321}; 4 players -> {0, 319, 480, 321}.
  Guard is `game_engine_force_single_screen() || cinematic_in_progress()`.
  Colour is `0xFF000000` through `draw_quad(rectangle2d *, pixel32)`
  (prototype copied from `source/cutscene/cinematics.c:129`, which owns it).
notes: HCEA names this `interface_splitscreen_render` (@0x836A6D90,
  `src/blam/interface/interface_splitscreen_render.c`). **Rename proposal for
  the orchestrator** -- I did not rename it (needs config/symbols.json).

**CAUTION for other workers:** writing this file with a bash heredoc
(`cat >> file <<'EOF'`) SILENTLY EATS one level of backslashes, so
`"c:\halo\..."` becomes `"c:\halo\..."` and the assert's `.rdata`
string mangles differently (`??_C@_0CB@GJIBJEIH@c?3haloSOURCE...`). Use the
Edit tool for any line containing a path literal.

### _code_000ce320  (padded 448 bytes)  [rename proposal: interface_get_weapon_hud_index]
status: RESIDUAL (size EXACT, relocations EXACT, 144 target vs 145 candidate
        instructions, ONE construct differing)
gate line: `residual    448  _code_000ce320  [sha]` (plain unit gate, no alias)

**RENAME PROPOSAL (orchestrator action, NOT done here).** This function is
`interface_get_weapon_hud_index` -- HCEA @0x836A6630,
`src/blam/interface/interface_get_weapon_hud_index.c`. Per the brief I kept the
January placeholder name in the source and did NOT touch `config/symbols.json`.
The proposed edit is the single line
`{ "file_offset": 844576, "flags": 32, "name": "_code_000ce320" }`
-> `"name": "_interface_get_weapon_hud_index"`, applied together with a
same-commit identifier rename in `source/interface/interface.c` (one
occurrence, line 463). Doing only one half leaves the tree inconsistent, which
is why I did neither.

signature: `long interface_get_weapon_hud_index(real *flashlight_power)`
  (one argument; `_interface_draw_screen` calls it as
  `lea eax,[ebp-4] / push eax / call / add esp,4` and then does
  `fld dword ptr [ebp-4]`, proving `real *`).

shapes tried (7):
  1. `unit_get(...)` nested inside the `TAG_BLOCK_GET_ELEMENT` block argument
     -> `size 464!=448`: cdecl right-to-left made
     `unit->unit.parent_seat_index` evaluate BEFORE the nested
     `object_get_and_verify_type` call, so VC7 cached it in `di`; that stole
     the third callee-saved register and spilled `weapon_hud_index` to the
     stack (2 stack slots, `sub esp,8`, `cmp ...,-1` immediates everywhere).
  2. declaration order swap (flashlight first) -> byte-identical (declaration
     order is inert, as documented).
  3. **hoist the parent lookup into its own local**
     `struct unit_datum *parent = unit_get(unit->object.parent_object_index);`
     -> the seat index is now read AFTER that call, VC7 cannot keep it in a
     register across the call, `edi` frees up, `weapon_hud_index` lands in
     EBX exactly like January (which is why January can write
     `cmp word ptr [esi+0x2a0], bx` and `cmp eax, ebx`). Size dropped to the
     exact 448.
  4. init order: `long player_index = local_player_get_player_index(...);`
     FIRST, then `weapon_hud_index = NONE`, then `flashlight = 0.0f`
     (HCEA has this order) -> reproduces January's
     `call / or ebx,-1 / add esp,4 / cmp eax,ebx / mov [ebp-4],0` schedule.
  5. explicit early `return weapon_hud_index;` in the `hud_index != NONE` arm
     -> WORSE (`size 432!=448`, 137 instructions); reverted.
  6. `flashlight` split declaration/assignment -> byte-identical.
  7. (kept) shape 3+4.

first divergence / reopen criterion:
  the ONLY remaining difference is how VC7 copies the `real` out-parameter at
  the two duplicated exits:
  ```
  January :  mov edx,[ebp-4] / mov ebx,eax / mov eax,[ebp+8] / pop edi / pop esi
             / mov [eax],edx / mov eax,ebx        <- 4-byte INTEGER copy
  ours    :  fld dword ptr [ebp-4] / mov edx,[ebp+8] / pop edi
             / fstp dword ptr [edx] / pop esi     <- x87 copy, and the
             following `mov ebx,eax` becomes DEAD because EAX still holds the
             return value.
  ```
  Every other instruction, the frame, the register assignment, all 24
  relocations and the padded size already agree. January never touches the
  FPU in this function; it copies `flashlight` with `mov` at all three points
  (`mov [ebp-4],0`, `mov [ebp-4],ecx`, `mov [eax],edx`). We match the first
  two and diverge only on the third. Reopen with whatever makes VC7 spend a
  GPR (rather than the x87 stack) on `*flashlight_power = flashlight;` at a
  point where EAX must be reloaded -- e.g. original January source, or an
  exact sibling that copies a `real` through a pointer parameter at a
  function exit.

evidence: HCEA `interface_get_weapon_hud_index.c` gives the complete
  semantics; every struct field it names already exists in this repo
  (`player_datum.unit_index`, `unit_datum.object.parent_object_index`,
  `unit_datum.unit.parent_seat_index/current_weapon_index/
  integrated_night_vision_power`, `unit_definition.unit.seats` with
  `sizeof(struct unit_seat)==0x11C`, `_unit_seat_gunner_bit`==3 (`test cl,8`),
  `weapon_definition.weapon.interface_definition.hud_interface.index`
  (+0x48C)). The masks match too: `push 3` = `_object_mask_unit`,
  `push 4` = `_object_mask_weapon`, so `unit_get()`/`weapon_get()` are the
  right typed accessors (house rule 7).
  Locally defined (no header may be edited): `struct hud_scripted_globals`
  (copied verbatim from `source/interface/hud.c`, which owns the global) and a
  partial `struct hud_global_data` with `default_weapon_hud` at +0x2C0 so
  `.index` lands at +0x2CC, asserted with `offsetof` (the same partial-view
  style `source/cutscene/cinematics.c` already uses for this global).
  New includes needed (measured inert -- 14 exact held): camera/director.h,
  game/players.h, items/weapon_definitions.h, items/weapons.h,
  objects/objects.h, render/render.h, units/unit_definitions.h, units/units.h.
notes: `_interface_draw_screen` (928 B, still unwritten) calls this function,
  so its relocation will also name whichever symbol wins the rename decision.

### negatives worth not repeating
* `/Op` (float consistency) is **inert** for this whole unit: gating with
  `--cflag /Op` reproduces all 14 exact functions byte-for-byte and leaves the
  `_code_000ce320` residual unchanged. The unit's natural
  `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` flags are correct; do not chase a flag
  override for the `fld/fstp` vs `mov` float-copy divergence.
* Declaration order really is inert here (tested twice on
  `interface_get_weapon_hud_index`).
* The whole file compiles clean at `/W3 /WX` -- no implicit declarations, no
  truncation warnings.

## SUMMARY

unit `source/interface/interface`: **exact 6 -> exact 14**, and the final
gate line is
`== exact 14  residual 1  unwritten 3  (of 18 listed)`
(the residual is `_code_000ce320`, written but one construct short).
Newly EXACT, smallest first:

| function | padded bytes |
| --- | ---: |
| `_interface_initialize_for_new_map` | 96 |
| `_interface_get_real_argb_color` | 128 |
| `_interface_get_tag_index` | 128 |
| `_profile_graph_toggle` | 128 |
| `_code_000cf2f0` | 224 |
| `_interface_draw_bitmap_modulated_p32` | 352 |
| `_interface_draw_bitmap` | 384 |
| `_interface_draw_bitmap_modulated` | 448 |
| **total** | **1,888 padded code bytes** |

Plus the object's entire **0x18918-byte `.data` section** (100,632 bytes,
18 relocations) now compares EQUAL to January under
`coff_compare.section_infos_equal` -- the three `profile_*_value_count` /
`profile_*_values` tables were recovered from the January object's own bytes.

Still unwritten: `_code_000ce900` (272), `_interface_draw_screen` (928),
`_code_000cea10` (2272). Written but RESIDUAL: `_code_000ce320` (448) --
size-exact, relocation-exact, 144 vs 145 instructions, one construct from done.

### hand-off notes for the next worker on this object
* `_code_000ce900` (272 B) is a **static function with a VC7 custom calling
  convention**: it reads `dx`, `eax` and `ecx` as incoming register arguments
  (plus two `real` stack arguments at `[ebp+8]`/`[ebp+0xC]`) and switches on a
  26-entry jump table selecting a `global_real_argb_*` colour, then draws a
  debug line via `matrix4x3_transform_point` +
  `rasterizer_debug_immediate_line`. It can only be reproduced together with
  its caller (almost certainly `_code_000cea10`), because MSVC only picks that
  convention for a `static` function whose one caller it can see. Do not try
  it standalone.
* `_interface_draw_screen` (928 B) needs (a) the `_code_000ce320` rename
  decision above, (b) a locally-defined `weapon_hud_interface_definition`
  ('wphi') with a 0xB8-byte block at tag offset +0xAC and fields at element
  offsets 0x04, 0x24, 0x34, 0x40/0x44/0x48/0x4C/0x50, 0x6C, 0x6E, 0x70, 0x8C,
  0x8E, 0x90, 0x94..0x9C, and (c) a 0x38-byte
  `rasterizer_screen_effect_parameters` local. `source/interface/weapon_hud_interface_definition.h`
  is still a 23-line stub, so all of that must be declared inside
  `interface.c` (the `hud_globals_definition` style in
  `source/interface/hud.c` is the precedent).
* `_code_000cea10` (2272 B) is the profile-graph renderer. Its assert
  `drawingbuf_counts[index] < 512` is at **line 704**; it consumes the
  `profile_*_value` tables this session recovered, and it reads
  `profile_graph_values[i-1]` through negative displacements that csplit
  attributes to `_profile_graph_value_count` (the known csplit relocation-alias
  trap: `[edi + _profile_graph_value_count + 5]` is really
  `profile_graph_values[i-1].enabled`).
