# `director.obj` Jonas ordinary-C credibility recovery

## Result

This 2026-08-30 audit starts from canonical commit
`69324deca14d7c3d96da1c4894dda79f5bf9be4c`. It revokes one false exact claim:
`_director_initialize_for_new_map` was byte-identical only because its body
contained eight hand-transcribed MSVC assembly blocks. The blocks reproduced
January's register allocation, direct offsets, private call setup and loop,
including `_emit 07ch` / `_emit 006h` raw branch bytes. That is object-code
transcription, not source reconstruction.

The first ordinary-C checkpoint was intentionally left as an honest fuzzy
residual while the surrounding private camera-selection functions were absent.
The final checkpoint recovers that adjacent helper cluster as ordinary, typed
`static` C. VC7 then selects the target private conventions and inlines the
80-byte `director_choose_camera` wrapper into the new-map loop naturally.
`_director_initialize_for_new_map` is again strict exact, now as credible C
rather than as an assembly transcription, and its obsolete park is removed.

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

| Candidate | Source status | Padded bytes | Relocations | Normalized SHA-256 | Comparator result |
| --- | --- | ---: | ---: | --- | ---: |
| E00 | Rejected assembly transcription | 208 | 19 | `70b72e43999f76618be8ead0ece3d3a9facba546b477b5245a1f0d2f88e9c6c6` | strict exact by construction |
| E01 | Credible C before private helpers | 128 | 10 | `5b1d60ea43cdd981bff2360af7d12b07702fc9dc2e763aa4b5648ff28ae5d459` | fuzzy, 55.460316% |
| E02 | Accepted ordinary C with private helpers | 208 | 19 | `70b72e43999f76618be8ead0ece3d3a9facba546b477b5245a1f0d2f88e9c6c6` | strict exact |

The focused final frontier is 23 shared functions strict exact, zero shared
residuals, and five target-only functions. The target-only work is
`_director_load_camera`, `_director_process_variables`,
`_director_script_camera`, `_director_update`, and
`_director_update_controls`; the object therefore remains nonmatching despite
having no residual among its emitted candidate functions.

Fresh hardened-comparator fingerprints for the recovered cluster:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_director_set_camera` | 96 | 5 | `27260493d43b3908b642c32677faa981fef60c468f65fe27a87dcf1df009b54b` |
| `_director_initialize_variables` | 112 | 6 | `15325a6711d5aec31328b544cd4b2f175c0c998df3837428a042a0227da980f2` |
| `_director_rotate_cameras` | 304 | 29 | `ab1700365d8086726ed8d94fdbf559e3089e9a17f2ddd999dc366bf5aee1ca3c` |
| `_director_choose_game_perspective` | 208 | 14 | `eebacfc41b25a5d466d329405c34be2d8f1d61228cf3cda28c529631baac6656` |
| `_director_choose_camera_game` | 448 | 34 | `0fa471656b66b2288f8774447767dff33e20387c3cecd17d226f1209d84e595a` |
| `_director_choose_camera_editor` | 176 | 12 | `f0c6c1e6015d4c7c15881f2048572d894e9174b68441ac142b48a80d2ac27863` |
| `_director_choose_camera_script_camera_record` | 192 | 16 | `d8356655340a2f6e66c5fb7ac3357c26a1fbc8da92fd9f71943d05dfd174bc2f` |
| `_director_choose_camera` | 80 | 10 | `52648e02dce63aafebbb46898937f05d338a6bd6a5c7a55f0df50d9b3839b491` |
| `_director_initialize_for_new_map` | 208 | 19 | `70b72e43999f76618be8ead0ece3d3a9facba546b477b5245a1f0d2f88e9c6c6` |

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
inlined into the loop. Defining that wrapper and its three private callees in
source order lets VC7 reproduce the inline schedule without a forced-inline
attribute or manual expansion. The helper bodies use typed camera structures,
real enum constants, ordinary calls, and coherent gameplay/editor/script-record
control flow; none exists merely to satisfy an instruction sequence.

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

`struct observer_command` is now owned by `camera/observer.h` and replaces the
anonymous 0x68-byte Director command buffer. Its 0x68 size and the used
`focus_position`, `focus_distance`, and `forward` offsets are target-backed.
The previously missing `following_camera_update` and `editor_camera_update`
declarations are placed in their owning camera headers rather than locally in
`director.c`.

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

`director_get` and the eight recovered Director helpers are defined `static` in
`source/camera/director.c`; none leaks into the public header. Their matching
private conventions arise from VC7's ordinary whole-TU optimization and are not
declared, staged, or forced. The corresponding January symbols are marked
`"static": true` in `config/symbols.json` so split ownership reflects those
private ABIs.

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
  `src/blam/camera/director_*.c` helper files corroborate the semantic helper
  mapping, source dependency order, and field names. They were treated as
  donors rather than verbatim source: raw decompiler offsets, tautological
  conditions, duplicated includes, generic integer types, and donor-marked
  deviations were replaced only where January and authenticated types support
  the ordinary source form.
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

The successful recovery confirms the correct lever: complete the authenticated
ordinary-C translation-unit context, then let VC7 choose inlining and private
conventions. A byte match from nonsensical logic or a higher fuzzy score alone
would still not be evidence of credible source.

## Validation record

The target profile is XDK 3911, compiled through the repository's production
graph with `/O2 /Oy- /DDEBUG /Dxbox`. The audit uses the hardened function
comparator, which requires code bytes and relocation identity rather than an
objdiff percentage.

Recorded gates:

- regenerated the January split object after the private-symbol metadata edit,
  then rebuilt `build/base/source/camera/director.obj`;
- focused residual frontier: one unit measured, 23 shared functions strict
  exact / 3,152 padded bytes, zero shared residuals, five target-only functions,
  zero comparison errors, and zero owner-name candidates;
- direct hardened comparison of all nine recovered helper/new-map owners: all
  strict exact, with the fingerprints recorded above;
- scoped fake-match scan of all five changed source/header files: zero findings;
- parked-function validation after removing the obsolete Director entry:
  12 active, zero stale, zero invalid;
- `config/symbols.json` and `config/parked.json` parse as valid JSON;
- `git diff --check`: pass.

Focused production validation:

- `halobetacache_build` and `libcmt_build`: pass; 13 dependent translation
  units rebuilt successfully after the camera-header ownership changes.
- Candidate COFF symbol audit: no `point_from_line3d` symbol or COMDAT.

Repository-wide semantic, campaign-board, and tooling-suite totals from the
earlier checkpoint are intentionally not repeated as current results. They were
not rerun after this final helper wave; canonical integration must regenerate
those aggregate reports.

Reproducibility identities:

- Final `director.c` Git blob:
  `d7e431189b87d7c2bf3ec543123742d8c1dfcb3a`.
- Final `director.h` Git blob:
  `3551988d3b023b6317c748774b12368322d9f01d`.
- Final `observer.h` Git blob:
  `e2a2c8fea2f33e07dc9d774e568276fc2af2fc7f`.
- Final `editor_flying_camera.h` Git blob:
  `0e737c0194cd67ad1faff0b11b888d1a559a043b`.
- Final `following_camera.h` Git blob:
  `bdda660a495c8f16274fb8bc09e4c6f77b19f4fe`.
- Final source SHA-256:
  `625dc48866f94f2203cf09f27c96e6f0025986defee9917e57733084baedb232`.
- Final header SHA-256:
  `965be8057d1d58a89fcf2d1b41f6a7b95020959d2de27783be7d8c2ce3316f14`.
- January split object SHA-256:
  `01504785032d98d69acf084639cfea9f8f775f7c3e65285310684bd14fa82151`.
- Candidate raw object SHA-256:
  `dcb629386c2553212602ef1462df67b7f2c881ed67cfec8554ead91a748aeb1c`.

Raw COFF hashes include producer metadata; the normalized per-function hashes
in the result table are the stable comparison identities.
