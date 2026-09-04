# `damage.obj` `render_debug_object_damage` recovery (2026-09-04)

## Result

The natural typed reconstruction of
`source/objects/damage.obj::_render_debug_object_damage` is strict exact. It
adds 455 meaningful bytes, 464 padded bytes, 34 ordered relocations, and one
function. The focused Damage gate advances from 22/34 to 23/34 exact
functions, with zero residual functions.

| Evidence | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `455 / 464` | `34` | `6c1727051cfb5dcf1a6f60b0d33a293558d57e72b7ad51a1b82fc06fe5c08f97` |
| Retained candidate | `455 / 464` | `34` | `6c1727051cfb5dcf1a6f60b0d33a293558d57e72b7ad51a1b82fc06fe5c08f97` |

The meaningful stream ends with `ret` at section offset `+0x1C6`; the nine
bytes from `+0x1C7` through `+0x1CF` are January's alignment padding.

## Baseline, instructions, and provenance

This lane was created from canonical commit
`4453bedd7b6271763e7411521caa5f80d65cb9e7` in the isolated worktree
`worktrees/damage-render-debug-20260904` on branch
`agent/damage-render-debug-20260904`. The canonical checkout was not edited.

Before source work, all five existing Damage ledgers were read, including
`damage_obj_small_vitality_leaves_20260904.md`. The latest historical paired
`CLAUDE.md` and `AGENTS.md` at
`76ed248678d92b401b8ff73913d756050129481f` were also read in full; the two
paths resolve to the same Git blob, `1dfb833913b42c15eb3f169309171d1f167c5716`.

The immediately prior ledger recorded an ordinary typed shape that once
reached the full 464-byte envelope and all 34 relocations, but no candidate
source was retained. The function was therefore reconstructed from the
January COFF rather than resurrected blindly. The local Stian and Pastudan PC
lifts corroborate the behavior, and HCEA corroborates the semantic debug
target name, but none is used as byte authority.

January fixes the complete topology:

- gate the display on `debug_damage`;
- copy `render.camera.window_bounds` and move the text rectangle 320 pixels
  along `x0`;
- print the point-and-space prompt when no object is selected;
- otherwise resolve the selected object through the typed accessor and format
  its tag basename, body vitality/current/recent damage, and shield
  vitality/current/recent damage;
- preserve January's stale-object branch, which clears the selected datum and
  continues to the common draw path without inventing a replacement path;
- draw in the project white color with the normal draw-string and rasterizer
  APIs;
- while Space is held, ignore the local player's unit, scale the camera
  forward vector by 50 world units, and test the vector against front-facing
  surfaces and objects;
- assert the object collision result and retain the hit object index.

The retained definition uses the project `boolean`, `real`, vector,
rectangle, collision-result, datum, enum, and `FLAG` types/macros. Object and
tag access go through their typed project accessors. The target symbol has
external COFF storage class 2 and function type `0x20`; its non-static owner
prototype remains in `damage.c`, because no other translation unit references
the currently owner-local API. Keeping it out of the broad `damage.h` include
surface also preserves the protected Units compiler schedule. The one-byte
link-common `debug_damage` symbol has no authenticated public owner, so its
ordinary `extern boolean` declaration is likewise local to the owning
translation unit.

## Exact assertion spelling

`match_assert` stringizes its expression. January owns the diagnostic literal
`collision.type==_collision_result_object`, without spaces around `==`, and
relocates to the corresponding `??_C@_0CJ@...` string owner. The usual spaced
source spelling produces a different diagnostic string and relocation
identity. Retaining January's authenticated expression spelling is therefore
semantic source recovery, not compiler steering.

## Ordered relocation proof

The target and retained candidate have the same relocation offsets, types,
addends, and normalized destinations:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x00A` | `DIR32` | `debug_damage` |
| `+0x017` | `DIR32` | `render + 68` |
| `+0x01D` | `DIR32` | `render + 72` |
| `+0x025` | `DIR32` | `damage_globals + 72` |
| `+0x038` | `DIR32` | no-object prompt string |
| `+0x049` | `REL32` | `_snprintf` |
| `+0x059` | `REL32` | `object_try_and_get_and_verify_type` |
| `+0x0A8` | `REL32` | `tag_get_name` |
| `+0x0B1` | `REL32` | `strrchr` |
| `+0x0BA` | `DIR32` | vitality format string |
| `+0x0CB` | `REL32` | `_snprintf` |
| `+0x0D6` | `DIR32` | `damage_globals + 72` |
| `+0x0E5` | `REL32` | `draw_string_set_format` |
| `+0x0EB` | `DIR32` | `global_real_argb_white` |
| `+0x0F1` | `REL32` | `draw_string_set_color` |
| `+0x107` | `REL32` | `rasterizer_draw_string` |
| `+0x10E` | `REL32` | `input_key_is_down` |
| `+0x122` | `DIR32` | `render + 8` |
| `+0x131` | `REL32` | `local_player_get_player_index` |
| `+0x137` | `DIR32` | `player_data` |
| `+0x13E` | `REL32` | `datum_get` |
| `+0x14A` | `DIR32` | `render + 28` |
| `+0x153` | `DIR32` | `50.0f` |
| `+0x162` | `DIR32` | `render + 32` |
| `+0x167` | `DIR32` | `render + 16` |
| `+0x16D` | `DIR32` | `50.0f` |
| `+0x17B` | `DIR32` | `render + 36` |
| `+0x181` | `DIR32` | `50.0f` |
| `+0x189` | `REL32` | `collision_test_vector` |
| `+0x1A3` | `DIR32` | January Damage source-path string |
| `+0x1A8` | `DIR32` | collision assertion string |
| `+0x1AD` | `REL32` | `display_assert` |
| `+0x1B4` | `REL32` | `system_exit` |
| `+0x1BF` | `DIR32` | `damage_globals + 72` |

## House-rule audit

The new no-argument declaration and definition put `void` on its own line,
the definition has an explicit terminal `return`, and each multi-line call
keeps one argument per line. Live identifiers are semantic. There is no
assembly, volatile/register steering, barrier, pragma, forced inline/noinline,
dummy dependency, raw address or byte-offset access, byte emission,
representation pun, new alias, fake branch, compiler change, or tool/admission
exception. The rebuilt Damage object contains no `point_from_line3d` string or
symbol, so the January inline/COMDAT schedule is preserved.

## Verification

- Full `ninja halobetacache_build libcmt_build semantic_progress progress`:
  pass from the final source.
- Focused Damage gate: **23 exact / 0 residual / 11 unwritten**, from a
  baseline of 22 exact / 0 residual / 12 unwritten.
- Focused function gate: `_render_debug_object_damage` strict exact at 464
  padded bytes and 34 relocations.
- Rename-stable whole-tree snapshots: 8,245 functions, exact count 6,130 to
  6,131; **+1 function / +464 padded bytes / zero regressions**.
- Campaign progress: 885,642 to 886,097 meaningful code bytes and 6,082 to
  6,083 functions; **+455 meaningful bytes / +1 function**.
- Semantic report: 473 units scanned, 6,513 functions evaluated, 6,140
  semantic exact, 6,156 accepted exact, and zero unit errors.
- Units protected sentinel: complete focused gate remains
  **189 exact / 0 residual / 0 unwritten**; in particular,
  `_unit_preprocess_node_orientations` remains exact at 1,920 padded bytes.
- `python -B -m tools.parked_functions`: 230 active / 0 stale / 0 invalid.
- `python -B tools/audit_object_admission.py`: zero candidates,
  contradictions, or revocations; the two inherited reviewed rejections are
  unchanged.
- Changed-file fake-match scan: zero review leads.
- `python -B -m pytest tools -q -p no:cacheprovider`: **261 passed**.
- Rebuilt `damage.obj` fixed-string scan: no `point_from_line3d`.
- Whole-source reference scan: no other translation unit references
  `render_debug_object_damage`.
- `git diff --check`: clean apart from Git's informational line-ending
  warnings.

The pre-edit TU manifest reports `_render_debug_object_damage` as the sole
`newly_exact` function and `changed_nonexact: []`. As expected for adding a
new code COMDAT plus its four constant-data owners, it also reports shifted
accepted-function evidence, new `.rdata`/`.debug$F` sections, and a changed
symbol set. No finding was suppressed. The rename-stable 8,245-function
comparison and complete focused Damage gate are the applicable no-regression
proof; both report the one intended gain and no loss.

Lane-local final fingerprints:

- regenerated January target object SHA-256:
  `E3139042F1F0D3CA250DEAEFA929277AD2A7A10245B2171C354D9289AC6812E9`;
- rebuilt candidate object SHA-256:
  `35CFDAEA490D38662A6393AC340BCA694DD32C33D5F552FB15DFDC8B25BA7CE8`;
- retained `damage.c` SHA-256:
  `0D9FFB744E8A5728BDED1743C98203C3DB553DE4D0D2A6963DA5576CD62D0847`;
- stable snapshots:
  `audit-artifacts/damage-render-debug-20260904/baseline.json` and
  `audit-artifacts/damage-render-debug-20260904/final.json`.
