# `first_person_weapons.obj` Opus reconciliation (2026-09-04)

## Scope and provenance

This packet selectively reconciles the frozen Opus first-person-weapon work
onto canonical commit `0a211a992bfbdf732727f42a18f00f3b812f1ada`.  The
donor was inspected read-only at
`C:/halo-worktrees/opus-small-families-30k-20260902`, tip
`eeedd72e5ba0fc1761519f68cad241fe606320e7`; the relevant donor commits are
`608c66380dca2d841b1e95150d1f88abe41a06d8` and
`8aed00b49b8b77188dc3aa2ce213ade5440f4e74`.

The donor was not cherry-picked.  Its source was reconstructed selectively
against the current canonical headers, symbols, January object, and campaign
rules.  The evidence reviewed before admission was:

- donor `scratch/reports/first_person_weapons.md`, including every retained
  function, attempted shape, rejected ABI workaround, and residual;
- donor `scratch/reports/naming_net-io-text.md`, whose section 13 proves the
  private global name `first_person_weapons` from January's own assertion
  expression;
- canonical
  `docs/object_matching_logs/first_person_weapons_obj_jonas_allocation_indexing_quartet_20260821.md`;
- canonical
  `docs/object_matching_logs/first_person_weapons_obj_jonas_render_only_wrapper_20260828.md`;
- the HCEA source family cited by the donor at
  `research-cache-halocea/src/blam/interface/first_person_weapon_*.c`, its
  complete `first_person_weapon.h` layout, and its state/message enums;
- January disassembly, padded section sizes, relocation order and identity,
  assertion paths/line anchors, and stable-section COFF hashes.

## Result

The focused hardened gate advances from **5 exact / 0 residual / 29
unwritten** to **26 exact / 4 residual / 4 unwritten** out of 34 functions.
The strict delta is:

- **21 newly exact functions**;
- **5,456 padded code bytes**;
- **5,293 meaningful code bytes**;
- **4 exact data bytes**, the TU-static `first_person_weapons` pointer.

The three switch-table functions which objdiff renders as fuzzy are recorded
in `config/semantic_matches.json`; hardened COFF comparison proves their
normalized bytes, padded size, relocation count, addresses, types and semantic
destinations exact.  Consequently the canonical progress path credits the
same 21 functions and 5,293 meaningful bytes rather than only objdiff's 18
ordinary-name matches.

| exact function | meaningful | padded | relocs | normalized SHA-256 |
|---|---:|---:|---:|---|
| `_first_person_animation_type_from_weapon_state` | 261 | 272 | 25 | `ea1838b37d9d2a3d39b80a5118d966f264e0daecbff3a47c9f41a6968006b499` |
| `_first_person_weapon_adjust_light` | 203 | 208 | 7 | `75df5e97df5e68852cc84f94818645a421c8f8daff8e58cb3e0b5c0c728283a3` |
| `_first_person_weapon_build_node_matrices` | 1,368 | 1,376 | 66 | `6a083d1a5e2118d6eae0004333e074682516d2c2c27eb9cbb792e9236fa29bd7` |
| `_first_person_weapon_center_flashlight` | 216 | 224 | 11 | `406f2da36705554daf7f8250ae7bea76c502a39f30039133d39e568f425673c2` |
| `_first_person_weapon_draw` | 642 | 656 | 29 | `0c5fda3be097bedcef44c950d55c758ce70a4018d8e20e9ca2ea7c6a49ca6192` |
| `_first_person_weapon_get` | 61 | 64 | 5 | `6b963a4aa367d587540d8fa9ad1ff905ef44123948117a68d2efbff5ebcbe181` |
| `_first_person_weapon_get_local_index` | 116 | 128 | 5 | `e63369b34066be7188c4b2cb5db82b3d962a23174330dbe24f6b0fe81efa5ca0` |
| `_first_person_weapon_get_marker_by_name` | 204 | 208 | 7 | `fc0a6c64acf41c5b30fb76446fea43b5df82177c9695121a618aa52193ebe53b` |
| `_first_person_weapon_get_marker_by_name_render` | 51 | 64 | 3 | `8bcdc31487a71d8577254b4b4651c0d535038cd645d50406952356638633c0e3` |
| `_first_person_weapon_index_from_unit_index` | 57 | 64 | 3 | `18e9bcc2d90fbca32d12f8d7ee4509819f0ee8936682be0ad57f8667ec0d683a` |
| `_first_person_weapon_index_from_weapon_index` | 94 | 96 | 4 | `03e9e7475e65e88d0f6cfa7af02387d6cf5a22141b2f31eb73d28108d66495c2` |
| `_first_person_weapon_message_from_unit` | 75 | 80 | 4 | `79e12a80e6e0aaf223787ed8192dc648c1eae83c49a8f3072d812a855cf9ae3d` |
| `_first_person_weapon_message_from_weapon` | 49 | 64 | 3 | `21ef805d054b11ed75d2a8823ea1cbbd5d09adedb8432917d17e7aaba4a3965b` |
| `_first_person_weapon_predict` | 116 | 128 | 8 | `e10d3a88af52c171beb5cc11f6ab09305702fd85d5df549fa10a6a8bed500854` |
| `_first_person_weapon_set_state` | 685 | 688 | 26 | `965964af9115913e36fa06e152828b2e857e7ac59e2cc0e6fdcff29ddebcb91f` |
| `_first_person_weapon_start_interpolation` | 176 | 176 | 9 | `462d109fb0d35daeb71799828a5f3e2248ed729400ebb6687f23881906efbcee` |
| `_first_person_weapon_state_from_weapon_message` | 189 | 192 | 19 | `f551efe027b59b504571439a15088c0065c23383298acbcdb85aeb04f10d3947` |
| `_first_person_weapons_initialize_for_new_map` | 73 | 80 | 3 | `e699349bdbd36871857ec3de663c151c14cf3ba5b8c559b625901a9ecf3d1757` |
| `_model_build_remapping_table_for_animation_graph` | 209 | 224 | 5 | `19dc4fd9067470dc2c805d54b4007fc140c1f4f8c43def7bdfa7fe716415d347` |
| `_model_remap_node_matrices_to_match_animation_graph` | 171 | 176 | 6 | `a77dde5e12f32cd92fa5307065e270d87b4eb32777adb495a2afa847333e4863` |
| `_weapon_play_first_person_weapon_sound` | 277 | 288 | 12 | `f66ceb6756561be4046f59b6f3c7325fe546c016bba4a549c2001a62a2920951` |

## Source-authentic integration decisions

The full recovered `first_person_weapon` layout uses engine `real`, vector,
orientation, animation-state and object types.  Object and tag lookup goes
through the subsystem getters and block-access macros. `weapon_get`,
`weapon_definition_get`, `unit_get`, `player_get`,
`animation_graph_definition_get`, `TAG_BLOCK_GET_ELEMENT`, and
`animation_graph_animation_index_get`; no raw `tag_get`, raw `object_get`, or
manual animation-block address cast remains.

January's global is defined as
`static struct first_person_weapon *first_person_weapons`.  This is not a
cosmetic rename: the file-static linkage permits VC7 to retain and reuse the
pointer across field stores, producing January's 80-byte
`first_person_weapons_initialize_for_new_map`.  External linkage reloads the
pointer and does not match.  January itself embeds the assertion expression
`"first_person_weapons"`, so the semantic name is first-party evidence rather
than an invented replacement for `bss_00453cc8`.

Private functions use their recovered semantic names and true static linkage;
the externally called `first_person_weapon_message` remains public because its
two January wrappers use the ordinary cdecl stack boundary.  Public
first-person prototypes live in `interface/first_person_weapons.h`, and former
ad-hoc declarations in `interface.c`, `render.c`, `render_objects.c`, and
`units.c` were replaced by that owner header.  Effect entry points were added
to `effects/effects.h`.

The two previously missing `model_animations.c` declarations live in the new
owner header `models/model_animations.h`, which is included by both the owner
and this consumer.  Initially placing them in the broad legacy
`model_animation_definitions.h` caused an otherwise unrelated exact
`render_sky` body to change under VC7 despite identical semantics.  Moving
them to the narrow owner header restored `render_sky` to 1/1 exact while
preserving `model_animations` at 16/32 and `first_person_weapons` at 26/34.
This is the measured transitive-header blast fix, not a suppression.

Small animation/render packet layouts remain TU-private.  Moving recovered
struct definitions into broadly included headers is known to perturb VC7's
definition-position-sensitive allocation even in functions which never name
the added type.  Every private layout used here has a compile-time size or
offset check against January.

The reconstruction retains several independently evidenced January quirks:
the redundant animation-graph lookup in `first_person_weapon_draw`, the local
player zero state read in the needler settle path, and the message-from-unit
inventory-slot argument.  These are called out by HCEA and independently
visible in January's loads/calls; they are not nonsensical source invented to
obtain a coincidental match.  Likewise, the zero-count polarity in
`weapon_play_first_person_weapon_sound` reproduces January's actual branch
order and its latent null-path bug.  No equivalent-looking behavior was
admitted without disassembly and source-family support.

## Honest residuals and unwritten functions

Four complete, coherent functions remain fuzzy and are parked with their full
measurements in `config/parked.json`:

| function | target / base padded | target / base relocs | objdiff | reason |
|---|---:|---:|---:|---|
| `_first_person_weapon_message` | 544 / 544 | 20 / 20 | 79.60345% | VC7 local-width and switch scheduling |
| `_first_person_weapon_render_update` | 176 / 192 | 14 / 14 | 92.28070% | private visibility-helper register convention |
| `_first_person_weapon_set_visibility` | 160 / 176 | 12 / 12 | 75.72916% | honest `short, boolean` signature does not recover January's two-register private ABI |
| `_first_person_weapon_switch_weapons` | 608 / 544 | 26 / 20 | 76.96774% | January inlines the smaller private visibility helper; the honest candidate remains a call |

The four unwritten target sections remain `_code_000cd610` (96 padded bytes),
`_code_000cd920` (368), `_code_000cda90` (1,536), and
`_first_person_weapons_update` (240).  No completion label is claimed for the
object.

Rejected donor/experimental shapes include a false `long` local-player
parameter used only to coerce the private register ABI, invented long message
and shotgun-state locals, forced inline annotations, duplicated or dummy
branches, raw structure offsets, volatile/register forcing, barriers,
pragmas, assembly, undefined representation tricks, and any completion based
only on objdiff's fuzzy score.  The broad-header placement described above was
also rejected because the stable sweep proved a real regression.

## Canonical replay

The isolated packet was replayed after the admitted profile timer packet at
canonical commit `43bc9e074`.  The first replay exposed one newer-canonical
interaction that the older isolated base could not see: replacing the local
`first_person_weapon_message_from_unit` declaration in `units.c` with an
include of the otherwise-correct owner header changed VC7's allocation
schedule in `_unit_preprocess_node_orientations`, reopening that 1,920-byte
function.  A bounded narrow-header experiment containing the identical
prototype produced the same regression.  Restoring January's declaration
position restored `units.obj` to **189/189 exact**.

The public declaration also remains in the proper owner header,
`interface/first_person_weapons.h`.  The redundant declaration in `units.c`
is therefore retained as a measured January/C2 schedule exception, not as the
sole or authoritative prototype.  This is preferable to accepting a proven
whole-tree regression or introducing conditional/preprocessor source whose
only purpose would be to steer the compiler.

After that correction, the current-tree stable sweep reproduces the full
isolated result: **21 gains / 5,456 padded bytes / zero regressions** across
8,245 target function sections.  Canonical progress moves from **850,394** to
**855,687 / 2,198,102 meaningful code bytes** and from **5,934** to
**5,955 / 11,060 exact functions**.  Exact data moves from 2,007,666 to
**2,007,670 bytes**; linked objects remain **391/833**.  The combined semantic
report contains 6,024 accepted exact functions and zero unit errors.

## Verification

Verification was performed in an isolated worktree with an independent
`build/` and `split/`; the frozen donor and canonical working tree were never
mutated.

- Focused gate: `26 exact / 4 residual / 4 unwritten`.
- Full `ninja all_source build/report.json build/semantic_report.json`:
  473 semantic units scanned, 5,955 semantic exact, 165 hidden exact,
  5,972 accepted exact, zero unit errors.
- Stable before/after sweep over all 8,245 target function sections:
  **21 gains, 5,456 padded bytes, zero regressions**.
- Progress baseline/after: code `843,658 / 5,882` to
  `848,951 / 5,903`; data `2,001,721` to `2,001,725`.
- `python -m pytest tools`: **261 passed**.
- Park audit: **192 active, 0 stale, 0 invalid**.
- Fake-match scan over all nine touched production source/header files:
  **0 review leads**.
- Object admission audit: **0 candidates, 0 contradicted, 2 inherited
  rejections** (`player_rumble` and `models`); no completion label is added by
  this packet.
- Link symbol dump of rebuilt `first_person_weapons.obj`: no
  `point_from_line3d` symbol or COMDAT.
- `git diff --check`: clean.
- Canonical protected bitmap hashes remained
  `AB6002BF2BAECDBC18C7B308BB7846C58FBCF1C7C72F586621D2B1F271D49140`
  (`bitmap_utilities.c`) and
  `B435F38E406C7DCDB60B4DF12E78BCF98D1E4B0E57FE2AB2240BC78F799EBA93`
  (`bitmaps.h`).
