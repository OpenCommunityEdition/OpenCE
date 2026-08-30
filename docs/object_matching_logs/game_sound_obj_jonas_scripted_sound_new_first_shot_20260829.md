# `game_sound.obj` scripted-sound first-shot boundary (2026-08-29)

## Scope and authority

This ledger records the single frozen production candidate for
`source/sound/game_sound.obj::_scripted_sound_new`. The candidate was a strict
miss and was removed in full. The production source is restored to Git blob
`0d64d0e0e957db61fa3621d05fa252efafcd7b64`; no source or header change is
retained, and the object remains at ten exact functions, zero residuals, and
21 unwritten functions.

The lane began at local campaign commit
`954af4d7fc8c5c8ad49cd0c9e1e7d2bb5a0457ac`. Before editing, both existing
game-sound ledgers were read completely:

- `game_sound_obj_jonas_scripting_leaf_wave_20260821.md`;
- `game_sound_obj_jonas_public_leaf_pair_20260828.md`.

The applicable campaign, object-ledger, audit, tooling, tree, and authenticated
Stian `CLAUDE.md`/`AGENTS.md` instructions had already been read in this
isolated worktree. History contained no prior emitted candidate for this
function. The newer public-leaf ledger mapped it but deliberately left it
unwritten because of union/decompiler and control-shape risk.

January PC COFF is authoritative. Target object
`build/split/source/sound/game_sound.obj` has raw SHA-256
`d69fd3772b48a1d483d0c80af8703210b85bc65d2dbdda1402c0e542ad54168`.
The compiler is the unchanged XDK 3911 Microsoft C/C++ compiler on the normal
production Ninja edge.

## Authenticated source evidence

The HCEA tree at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains the public body
`src/blam/sound/scripted_sound_new.c`, Git blob
`2fe83d90ecbe8978759925d3530efb635d5cbc78`. HCEA authenticates the natural
high-level topology: validate the definition, stop the previous scripted
instance, update its predicted end time, clamp scale, choose attached versus
unspatialized creation, notify the attached object, and store the runtime
sound handle.

January disassembly and canonical typed declarations fix the PC-specific
details that supersede HCEA's later layout:

- `struct sound_definition` owns `longest_permutation_length` at `+0x84`,
  `scripting_time` at `+0x90`, and `scripting_sound_index` at `+0x94`;
- `struct object_marker` owns a named `node_index` and typed
  `real_matrix4x3 node_matrix`, including named `forward` and `position`;
- the attached call is the existing six-argument
  `object_impulse_sound_new` public declaration;
- the no-object path is the existing two-argument
  `unspatialized_impulse_sound_new` public declaration;
- the remaining calls use the ordinary public cdecl declarations for
  `tag_get`, `sound_stop_impulse`, `game_time_get`,
  `object_get_marker_by_name`, and `object_type_notify_impulse_sound`.

The frozen candidate was readable C89 with one parameter per line, explicit
terminal `return;`, typed named-field access, and natural `PIN`/tick constants.
It used no assembly, `volatile`, `register`, pragma, intrinsic, attribute,
barrier, raw address or byte offset, pointer/integer reconstruction,
representation pun, undefined behavior, byte forcing, compiler change, tool
exception, or admission exception.

## Frozen one-shot result

After a non-emitting `/Zs` type check, exactly one normal production object
edge ran and its result was immediately preserved as
`build/audit/game_sound_scripted_sound_new_first_shot_20260829.obj`. Its raw
SHA-256 is
`4087acc799af3ce660bf5f0d307c2434d815a0a68b132df54f8e016218232ea4`.

| Measure | January target | Frozen candidate |
| --- | ---: | ---: |
| Instruction extent | 352 | 350 |
| Padded COMDAT extent | 352 | 352 |
| Relocations | 12 | 12 |
| Normalized SHA-256 | `4c5fbb78507d640978df233b21eeffce0c7dc3ee49f37bbc4a362055a140eb51` | `666bb89f52eae460fa96023cb8bbae9fe1083984a1ec888ae9126a07e37a890c` |

All twelve relocation types and destinations are natural, but their complete
ordered schedules differ:

| Destination | Target offset | Candidate offset |
| --- | ---: | ---: |
| `_tag_get` | `+0x1F` | `+0x1F` |
| `_sound_stop_impulse` | `+0x2D` | `+0x30` |
| `_game_time_get` | `+0x35` | `+0x38` |
| `__real@00000000` | `+0x44` | `+0x47` |
| `__real@3f800000` | `+0x7B` | `+0x77` |
| `_unspatialized_impulse_sound_new` | `+0x14A` | `+0x97` |
| `"head"` | `+0xA3` | `+0xBA` |
| `_object_get_marker_by_name` | `+0xA9` | `+0xC0` |
| `_global_origin3d` | `+0xDD` | `+0xF7` |
| `_global_forward3d` | `+0xF1` | `+0x10B` |
| `_object_impulse_sound_new` | `+0x11B` | `+0x135` |
| `_object_type_notify_impulse_sound` | `+0x130` | `+0x147` |

The decisive boundary is control layout, not missing types or ABI. The natural
HCEA spelling tests the no-object case first, so VC7 emits the
unspatialized call before the attached-object path. January emits the attached
path first and places the unspatialized call in the final branch. The
candidate also homes the definition pointer through the first argument slot,
where January keeps it in `EBX`. Closing either difference would require a
second spelling or branch-order/codegen experiment, which this lane forbids.
The candidate therefore received no credit and must not be retried without
new authenticated PC source evidence that independently fixes the original
control spelling.

## Restoration and gates

The candidate body and its two added includes were inverse-removed immediately.
The restored source hashes to the exact committed blob named above. A normal
single-object rebuild removed `_scripted_sound_new` again, and hardened COFF
comparison reconfirmed all ten inherited exact siblings. Focused campaign
gate: ten exact, zero residual, 21 unwritten.

Final validation passed:

- complete `halobetacache_build` plus `libcmt_build`: settled with no pending
  work after report regeneration;
- semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 hidden code bytes, 4,810 accepted exact, and zero
  unit errors;
- strict progress: 384/833 objects, 4,771/11,060 functions,
  606,187/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- object admission: zero candidates, zero contradictions, and zero
  revocations;
- parked validation: 12 active, zero stale, and zero invalid;
- protected Units `_unit_preprocess_node_orientations`: exact at 1,920 padded
  bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- tooling suite: 212/212 tests passed;
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`;
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`;
- `git diff --check`: clean, with the ledger as the only tracked delta.
