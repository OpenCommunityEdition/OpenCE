# `director.obj` Jonas ordinary-C credibility recovery

## Result

This 2026-08-30 audit starts from canonical commit
`69324deca14d7c3d96da1c4894dda79f5bf9be4c`. It revokes one false exact claim:
`_director_initialize_for_new_map` was byte-identical only because its body
contained eight hand-transcribed MSVC assembly blocks. The blocks reproduced
January's register allocation, direct offsets, private call setup and loop,
including `_emit 07ch` / `_emit 006h` raw branch bytes. That is object-code
transcription, not source reconstruction.

The function is now ordinary, typed C with no assembly or code-generation
coercion. It is intentionally reported as an honest fuzzy residual and remains
parked under `tu-context-optimization`; it receives no exact or semantic-exact
credit. Its obsolete entry in `config/semantic_matches.json` is removed so the
strict progress gate cannot continue crediting the former transcription.

The same source-level audit recovers the previously absent 64-byte private
accessor as `static struct director *director_get(short)`. VC7 assigns its
private SI convention naturally and emits all five target relocations exactly:
normalized SHA-256
`ca74e12a19a2f27ec8a62ac17e08e1042b3ad679ac9e3980249f257b1aa16035`.
No register convention is declared or forced.

The authenticated `director_camera_mode_names` and `variables` initializers
also reproduce the complete 136-byte `.data` section, including all five
string relocations, with normalized SHA-256
`2f47b267059a50f6bee1a0a581bf1c613cf52fc664109b56756d5e288a03c88f`.
The two camera-mode arrays reproduce the first 16 bytes of the target
`.rdata` section, including their natural alignment. The named 5.0f `friction`
value also reproduces its target bytes, but appears at candidate offset 0x10
rather than target offset 0x14 because the preceding 0.03f literal pool belongs
to a still-missing Director function. The other missing literals are 25.0f and
1.3f, so the honest candidate section is 20 bytes versus the target's 32. No
synthetic globals or padding were added to imitate those pools.

| Candidate | Source status | Padded bytes | Relocations | Normalized SHA-256 | Objdiff fuzzy |
| --- | --- | ---: | ---: | --- | ---: |
| E00 | Rejected assembly transcription | 208 | 19 | `70b72e43999f76618be8ead0ece3d3a9facba546b477b5245a1f0d2f88e9c6c6` | strict exact by construction |
| E01 | Accepted ordinary C | 128 | 10 | `5b1d60ea43cdd981bff2360af7d12b07702fc9dc2e763aa4b5648ff28ae5d459` | 55.460316% |

The 13 inherited sibling exact functions remain strict byte-and-relocation
exact, and `_director_get` is newly exact. The invalid assembly-backed exact
owner is therefore replaced in the strict count by one genuine ordinary-C
owner. The object remains nonmatching.

## Accepted source shape

The accepted routine performs only source-level work justified by the program:

1. Set the primary Director mode to `_director_mode_editor` when
   `game_in_editor()` is true, otherwise `_director_mode_game`.
2. Clear the authenticated `initialize_camera` flag.
3. For every local player, resolve the typed state through `director_get`, then
   clear `camera_change_pause`, `bored_time`, and `bored`.
4. Call `director_choose_camera(local_player_index, TRUE, FALSE)` and
   `director_initialize_variables(local_player_index)`.

The wrapper call is deliberate. January's 208-byte function contains the
mode-switch work of the separate 80-byte `_director_choose_camera` wrapper
inlined into the loop. The current translation unit declares that wrapper but
does not yet define its private camera-selection callees, so VC7 cannot
recover the original whole-TU inline and register schedule. Manually expanding
the switch, forcing registers, or spelling private ABI calls in assembly would
only recreate object code. The honest residual is parked until ordinary-C
helper definitions make that optimization context available naturally.

The January inline schedule remains otherwise untouched. In particular, this
change does not introduce or emit a `point_from_line3d` COMDAT.

## Authenticated names and types

`source/camera/director.h` now uses the real C-series `real`, `boolean`, `short`,
and `long` types already required by the ABI. The former overlapping
`director_player_globals[4]` model is replaced by the authenticated layout:

- one 0x3E8-byte `struct director_globals` containing `dtime`, `game_mode`,
  `initialize_camera`, and four 0xF8-byte `struct director` player states;
- the candidate common `_director_globals` symbol is now exactly 1,000 bytes,
  ending at January's separately named `director_camera_switch_fast` byte;
- `camera_mode_index`, `camera_proc`, `camera_data`, `seat_state`, `command`,
  and `debug_variables` now occupy their proven fields instead of anonymous
  padding.

The header also adds the DB-authenticated enum constants:

- `_director_mode_game`
- `_director_mode_netgame`
- `_director_mode_editor`
- `_director_mode_scripted`
- `_director_mode_script_camera_record`

Target offsets plus the HCEA type database authenticate the renamed fields:
`game_mode`, `initialize_camera`, `camera_change_pause`, `bored_time`, `bored`,
`debug_controls`, and `debug_input_scale`.

The same Director ordering, extents, calls, and HCEA evidence map ten private
address placeholders to authenticated names. January proves the remaining
0x40-byte owner is the bounds-checking typed state accessor; because its
original name was not present in the available database or donor trees, it is
named `_director_get` under the repository's established `<subsystem>_get`
convention, as explicitly permitted for unresolved private names:

| January address | Semantic symbol |
| --- | --- |
| `000747D0` | `_director_get` |
| `00074C10` | `_director_set_camera` |
| `00074C70` | `_director_initialize_variables` |
| `00074CE0` | `_director_process_variables` |
| `000750C0` | `_director_rotate_cameras` |
| `00075250` | `_director_choose_game_perspective` |
| `00075450` | `_director_choose_camera_game` |
| `00075610` | `_director_choose_camera_editor` |
| `000756C0` | `_director_choose_camera_script_camera_record` |
| `00075780` | `_director_update_controls` |
| `00075B40` | `_director_choose_camera` |

`director_get` is defined `static` in `source/camera/director.c`; its prototype
does not leak into the public header. Only the two still-external helpers called
by the recovered routine are declared in that C file's own prototype section.
No prototype was moved into an unrelated implementation file and no missing
definition was fabricated.

The Director-owned address-only data labels are also split at their proven
boundaries and named: `director_game_camera_modes`,
`director_script_camera_record_camera_modes`, `friction`,
`director_camera_mode_names`, and `variables`. The intervening unnamed floating
constants retain normal `__real@...` compiler names. HCEA database types and
the January bytes establish the 5-pointer name table and four 0x1C-byte
`director_variable_definition` records.

## Provenance

Evidence was ranked rather than copied indiscriminately:

- January target COFF, disassembly, and relocations are authoritative for
  layout, control flow, call order, object extent, and exactness.
- `research-cache/halocea-cseries-20260820/src/blam/camera/director_initialize_for_new_map.c`
  independently corroborates the typed reset-and-two-calls routine.
- `research-cache/halocea-cseries-20260820/src/headers/director_game_mode.h`
  supplies the database-authenticated enum names and values. The same tree's
  individual Director helper files corroborate the semantic helper mapping and
  field names.
- `research-cache/stian-halo-complete-history-20260828/src/halo/camera/director.c`
  and
  `research-cache/pastudan-halo-full-history-20260828/src/halo/camera/director.c`
  independently recover the same mode selection, per-player three-field reset,
  mode dispatch, and per-player initializer.
- `research-cache/punpckhdq-halo-full-history-20260828/source/camera/director.c`
  contains only address placeholders for this cluster and provides no stronger
  source to integrate.

The repository `CLAUDE.md`, the accessible donor-tree `CLAUDE.md` files,
`docs/matching_methodology.md`, `docs/fake_match_review.md`,
`docs/object_matching_logs/README.md`, and
`research/asm_credited_functions_audit.md` were read and reconciled before the
edit. Scoped Marathon-source searches found no analogous camera Director
implementation, so no Marathon behavior was inferred. Community discussion
that helper/math functions may sometimes contain original inline assembly does
not establish provenance for Director control-flow assembly and was not treated
as permission to retain it.

## Rejected shapes and do-not-repeat guidance

Do not reintroduce any of the following to improve the score:

- `_emit`, raw instruction bytes, inline or naked `__asm`;
- manually staged EAX/EBX/ESI/EDI values or comments that prescribe registers;
- direct Director byte offsets or a hand-maintained pointer stride;
- a manually expanded mode switch whose only purpose is to imitate the inlined
  wrapper;
- `volatile`, `register`, optimizer pragmas/barriers, forced-inline attributes,
  undefined behavior, fabricated branches, synthetic anchors, or byte patches;
- numeric enum cases where authenticated constants exist;
- address-derived private function or BSS names.

Any future retry must begin with ordinary-C definitions for the authenticated
helper cluster, then let VC7 choose inlining and its private conventions. A
higher fuzzy score alone is not evidence of better source.

## Validation record

The target profile is XDK 3911, compiled through the repository's production
graph with `/O2 /Oy- /DDEBUG /Dxbox`. The audit uses the hardened function
comparator, which requires code bytes and relocation identity rather than an
objdiff percentage.

Recorded gates:

- candidate build and direct function comparison for
  `_director_initialize_for_new_map`;
- exact-set comparison against the preserved E00 object: all 13 inherited
  sibling exact functions retained and `_director_get` newly exact;
- scoped fake-match scan of `source/camera/director.c` and
  `source/camera/director.h`: zero findings;
- parked-function validation: 13 active, zero stale, zero invalid;
- `git diff --check`: pass.

Final production validation:

- `halobetacache_build` and `libcmt_build`: pass; no work remained after the
  final Director replay.
- Semantic report: 470 units scanned; 4,987 functions evaluated; 4,877
  semantic exact; 130 hidden exact / 78,743 hidden code bytes; 4,887 accepted
  exact; zero ordinary rejected and zero unit errors.
- Strict campaign board: 384/833 files, 4,848/11,060 functions, and
  618,804/2,198,102 meaningful code bytes.
- Halo board: 282/468 files, 4,681/7,574 functions, and
  605,890/1,770,166 meaningful code bytes.
- Object-admission audit: zero candidates. Its sole contradiction/revocation
  is the inherited `source/ai/action_converse` completion label, unrelated to
  this wave.
- Tooling suite: 221 tests passed.
- Candidate COFF symbol audit: no `point_from_line3d` symbol or COMDAT.

Reproducibility identities:

- Final `director.c` Git blob:
  `4f7db666d586273dc38ac03d55793b4c011eeda9`.
- Final `director.h` Git blob:
  `fe455a79ad7ef4e4151eeeeab1f45b9457c78da7`.
- Final source SHA-256:
  `52b4b773c43f27209241fc2df8da73d6ad4f042cd82e621e4386fe03f3124a67`.
- Final header SHA-256:
  `9d3f64a45d6ac6ace89c215a229d58e49937b34b738ce380f537dd0e84262588`.
- January split object SHA-256:
  `0238797739ab317ba363419ab943d929bdab4173a39a8a6a92b64dc763cb70ba`.
- Candidate raw object SHA-256:
  `86d76476bf86426dbc3263135f80e00654e22446006f74fb25f258bb487a497d`.
- `build/report.json` SHA-256:
  `a64bcee6234f6ac5209f9067db4e7cfc702661b639d10501fc0d341f781513f4`.
- `build/semantic_report.json` SHA-256:
  `07229eada628261dfd37cfdaccb497bb855146e8fb3b46eb8e2a53ae5fe07707`.

Raw COFF hashes include producer metadata; the normalized per-function hashes
in the result table are the stable comparison identities.
