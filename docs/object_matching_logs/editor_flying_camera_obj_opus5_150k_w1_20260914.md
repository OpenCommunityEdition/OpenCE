# `editor_flying_camera.obj` park re-investigation (Opus5 150K house-clean lane, wave w1, 2026-09-14)

## Result

No real-file change. The unit stays at **16 exact / 5 residual / 0 unwritten** (guard passes, zero park drift).
Two of the five parks are proven strict EXACT in scratch candidates and are proposed for reopen; the other three
are reduced to owner rulings.

| Function | January padded/relocs | Outcome | Blocker / requirement |
| --- | ---: | --- | --- |
| `_editor_camera_orbiting_update` | 1120 / 49 | EXACT in scratch (P1) | none: one include line |
| `_editor_camera_update` | 304 / 26 | EXACT in scratch (P2) | `config/symbols.json` anchor rename |
| `_editor_camera_set_scripted` | 432 / 40 | EXACT in scratch with a cast | owner ruling on point/vector pointer casts + P2 |
| `_editor_camera_flying_update` | 1408 / 60 | EXACT in scratch with `point_from_line3d` | owner ruling on the `_point_from_line3d` COMDAT + P2 |
| `_editor_camera_set_position_and_roll` | 432 / 30 | 432/30 with a cast, still 3 x87 load-order blocks | cast ruling + new evidence |

## New evidence

1. **Implicit declaration.** `CL /Zs /W3` on the real file reports
   `C4013: 'director_inhibit_input' undefined; assuming extern returning int`. The TU never includes its owner
   `source/camera/director.h`. Without a prototype, the short argument is default-promoted with
   `movsx eax, word ptr [ebx]`. January instead emits `xor eax,eax / mov ax,word ptr [ebx]`, which is the same
   sequence the exact sibling `orbiting_camera.c:_orbiting_camera_update` produces (it includes `camera/director.h`).
   This was the only instruction difference in the orbiting park, so the park's "natural scheduling" premise is
   refuted.
2. **HCEX.pdb compiland record** (`DIA2Dump -compiland ..\..\..\build\x360\SHIP\halo\editor_flying_camera.obj`,
   dump kept at `scratch/workers/editor_flying_camera/hcex_compiland.txt`).
   - The editor-camera state is a set of **separate file statics**, not one aggregate. In definition order:
     `is_scripted`, `use_roll`, `initialized`,
     `editor_camera_focus` (`struct editor_camera_focus_definition`), [`speed`], `editor_camera`, `reset_all`,
     [`unit_focus`], `unit_offset` (`real_vector3d`), `camera_mode` (short), `local_player_index` (short),
     `last_scripted`, [`update_funcs`, `translate_funcs`], `persisted_cameras` (`struct persisted_camera_data[2]`,
     member `camera_data`).
   - January corroborates this independently: its assertion literals `update_funcs[camera_mode]` and
     `translate_funcs[camera_mode][_translate_from]` stringize a plain `camera_mode` variable.
   - `flying_update` has one aggregate local, `real_point3d new_position`.
3. **Measured VC7 layout law.** VC7 places explicitly zero-initialized statics in `.bss` in **definition order,
   with natural alignment**.
   - With `local_player_index` defined, the object's `.bss` is 124 bytes and every January offset is reproduced
     (+0, +1, +2, +4, +0x18, +0x1C, +0x20, +0x2C, +0x32, +0x38, +0x78). `local_player_index` is unreferenced here,
     and so is its January slot at +0x30.
   - Without it, `.bss` is 116 bytes and `_editor_camera_bump_speed` plus both translate functions lose relocation
     identity.
4. **Aggregate aliasing.** VC7 must reload `editor_camera_globals.camera` after pointer stores because the
   aggregate is address-taken (`&.focus`, `&.unit_offset`, ...). A non-address-taken `static struct flying_camera
   *editor_camera` is hoisted, which reproduces January's single camera fetch in `editor_camera_update` and in the
   `else` arm of `editor_camera_set_scripted`. It also removes the "use_roll load schedule" difference recorded in
   the flying park.

## Proposals

### P1: park-reopen `_editor_camera_orbiting_update` (no header or config change)

- Candidate: `scratch/workers/editor_flying_camera/reopen_orbiting.c`, which is the current real file plus
  `#include "director.h"` after `#include "camera_scripting.h"`.
- Whole-TU gate: 17 exact / 4 residual. All 16 baseline exact rows stay exact and the guard passes.
- `parkcheck`: only orbiting drifts, and it drifts to the target SHA `280e2c91cb77eeaf`.
- Owner census and fake scan are clean.

### P2: park-reopen `_editor_camera_orbiting_update` and `_editor_camera_update` (supersedes P1)

- Candidate: `scratch/workers/editor_flying_camera/cand_split.c`. It is the current real file plus:
  - the `director.h` include;
  - the aggregate replaced by the HCEX-ordered statics;
  - the authentic HCEX tag and member names.
- Config needed in the same commit: an in-place `config/symbols.json` line 22460 edit,
  `"name": "_editor_camera_globals"` -> `"name": "_is_scripted"`, followed by a csplit-only regen. The only consumer
  is `source/camera/editor_flying_camera.c`.
- Gate results:
  - With `--alias is_scripted=editor_camera_globals` standing in for the rename: 18 exact / 3 residual, guard passes,
    `_editor_camera_update` 304/26 SHA `3f3b95ee`. alndiff shows no differences and relocdiff shows 0 differing
    rows.
  - Without the rename: 3 exact. The source change and the config change must land together.
- Linkage disclosure: January and HCEX define `is_scripted` as static (no January PDB public at 0x31d438). The
  comparator's defined-noncode rule needs an external owner at offset 0 of a section that holds several statics.
  This is the same compromise the canonical `.data` anchor `editor_camera_speed` already makes. If the owner
  rejects it, the alternative is a comparator change that admits a name-identical static anchor.
- Parks to re-measure (bodies unchanged apart from identifier renames):

  | Park | size / relocs | SHA |
  | --- | ---: | --- |
  | `_editor_camera_flying_update` | 1408/60 | `a1561d2416e79cf1` |
  | `_editor_camera_set_position_and_roll` | 464/30 | `2115d1bd30c95e6b` |
  | `_editor_camera_set_scripted` | 464/42 | `2a4d19c78186eb46` |

- Owner census: no new external, COMMON or code owner. The candidate adds 11 static `.bss` labels, the same class
  as the existing static `.data` labels. Fake scan: 0 leads.
- Optional neutral cleanup: `cand_split_terminal.c` also includes `interface/terminal.h`, which fixes the
  `terminal_printf` C4013. It gives the same 18 exact rows and the same park hashes.

### Owner rulings (not landed)

- **`_editor_camera_set_scripted`.** January's frame is `sub esp,8`, and it pushes `_editor_camera_globals+0x20`
  directly as the point argument. With P2 plus `(real_point3d const *)&unit_offset`, the function is 432/40 SHA
  `925a9c06` EXACT (`ss_a1.c`). Representation casts were declined by the previous lane.
- **`_editor_camera_flying_update`.** January computes the center pointer lazily (`fadd [eax+0x50]; add eax,0x50;
  fadd [eax+4]; fadd [eax+8]`), which is the signature of an inline pointer argument. With P2 plus
  `point_from_line3d(&unit->object.bounding_sphere_center, &unit_offset, 1.f, &position)` in the unit branch, the
  function is 1408/60 SHA `1e60e849` EXACT (`t5.c`). That form emits the forbidden `_point_from_line3d` COMDAT.
  This adds 1,408 B to the pending COMDAT-policy ruling.
- **`_editor_camera_set_position_and_roll`.** January falls through into the no-camera path and passes the
  `real_euler_angles3d` pointer straight to `editor_camera_set_focus`, so it too needs a cast. Even with that cast
  (`spr_a1.c`), three x87 operand-load-order blocks remain:
  - the i-term of `cross_product3d`;
  - the k and j terms of `dot_product3d`.

  Swapping the `dot_product3d` arguments is byte-identical, so it does not help. The remainder is a tie; stopped.

## Rejected / measured-only shapes

| Shape | Result |
| --- | --- |
| t1: split without `local_player_index` | update EXACT, but 3 exact rows lost (116-byte `.bss`) |
| t6: `point_from_line3d` in both flying branches | EXACT, but guard fails |
| spr_a2: `dot_product3d` argument swap | inert |

## Reopen criteria

| Function | Reopen when |
| --- | --- |
| orbiting | P1 accepted |
| update | P2 accepted (symbols.json rename) |
| set_scripted | cast ruling + P2 |
| flying_update | `_point_from_line3d` COMDAT ruling + P2 |
| set_position_and_roll | cast ruling + new evidence for the roll-reference / dot evaluation spelling |
