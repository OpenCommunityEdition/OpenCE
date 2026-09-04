# `ai_profile.obj` complete-reconstruction admission review (2026-09-04)

## Verdict

The reconstructed code in `source/ai/ai_profile.c` is source-credible and is
approved for whole-object admission as `Matching`, conditioned only on the
orchestrator's final full-report/admission and zero-regression checks. All 23
January function sections are represented by ordinary C and the regenerated
candidate proves all 2,048 padded code bytes exact.

One source-authenticity blocker was found during this review and corrected in
the pending source before admission: the earlier candidate represented the
entire 236-byte initialized span as one `struct ai_meter_definitions` solely
because csplit grouped it under `_global_ai_meter_definitions`. Neither csplit
ownership nor adjacency proves an original C aggregate. The pending source now
uses the natural boundaries supported by the contents and consumers:

- a public 28-entry `global_ai_meter_definitions` array, 224 bytes; and
- a private three-pointer `ai_profile_render_spray_names` table, 12 bytes.

The private name is a reviewer-assigned semantic name, not a recovered PDB
name. `config/symbols.json` places that static boundary at January RVA
2,848,056 (`_global_ai_meter_definitions + 0xE0`). This avoids admitting a
match-oriented aggregate facade while preserving the real linked layout.

Audit base: canonical commit
`9a1a99f805e6a3df34327f4b6e4c20e6d123b66a`. No implementation, header, config,
build, commit, or push operation was performed by this independent review.

## Exact function evidence

The preserved focused object
`scratch/ai-profile-full-second-20260904.obj` has SHA-256
`aed2b41b1814a7848ab5246be34ba5bbfd69ca24ad4cbe0760d003af72cd5432`.
Against the pre-rename January split object (SHA-256
`314d8c8287d1088a7d4a7a26d73b0c392cb8eabbdeff5ee0432d075e143e9d14`),
all 23 functions agree in section size, normalized bytes, relocation count,
and relocation identity when the 15 private semantic names are paired with
their former `code_<address>` labels. Target and candidate each emit exactly
23 selection-1 (`IMAGE_COMDAT_SELECT_NODUPLICATES`) code sections with flags
`0x60501020`. There is no extra candidate code section, helper, thunk, or
inline body.

| Function | Padded bytes | Relocs | Normalized SHA-256 prefix |
| --- | ---: | ---: | --- |
| `_ai_profile_initialize` | 32 | 3 | `cb05d1c94e00cef8` |
| `_ai_profile_dispose` | 16 | 0 | `499f1f307c1cb989` |
| `_ai_profile_initialize_for_new_map` | 32 | 2 | `07cee23dd7eed965` |
| `_ai_profile_dispose_from_old_map` | 16 | 0 | `499f1f307c1cb989` |
| `_ai_profile_update` | 272 | 10 | `3607db5610cb1f67` |
| `_ai_profile_display` | 112 | 12 | `74bd8b766b356d09` |
| `_ai_profile_draw_string` | 144 | 8 | `ac2cddf40ada6da8` |
| `_ai_profile_change_render_spray` | 64 | 6 | `e4e3fc4f00585b05` |
| `_ai_profile_sample_encounters` | 16 | 1 | `df284e25050752fb` |
| `_ai_profile_sample_actors` | 16 | 1 | `df284e25050752fb` |
| `_ai_profile_sample_units` | 80 | 3 | `bfe634cf096f3c66` |
| `_ai_profile_sample_props` | 16 | 1 | `df284e25050752fb` |
| `_ai_profile_sample_swarms` | 16 | 1 | `df284e25050752fb` |
| `_ai_profile_sample_swarm_actors` | 80 | 3 | `728f7dc43e558130` |
| `_ai_profile_sample_swarm_components` | 16 | 1 | `df284e25050752fb` |
| `_ai_profile_render_encounters` | 96 | 9 | `5c1c723474778813` |
| `_ai_profile_render_actors` | 112 | 12 | `cdced00df1d62149` |
| `_ai_profile_render_swarms` | 96 | 9 | `5b0aaa56f70eb899` |
| `_ai_profile_render_props` | 144 | 15 | `c9103c4182447bed` |
| `_ai_profile_render_collisions` | 112 | 10 | `a86cc8fb0cb04ece` |
| `_ai_profile_render_paths` | 96 | 9 | `0924488a1c392555` |
| `_ai_profile_render_spray` | 336 | 17 | `c74475be2ca6298a` |
| `_ai_profile_render` | 128 | 16 | `eded64cae59526a9` |

The semantic names are supported by each function's role, the initialized
callback table, format strings, and call graph. The January PDB2.00 public
symbol census does not recover private names, so this review does not present
them as historical spellings. They satisfy the campaign rule to replace raw
address labels with meaningful subsystem-convention names, and the pending
symbol records correctly mark all 15 as `static`.

After the semantic symbol rename and natural data split, the regenerated
January object has SHA-256
`5949e8c538dce671434f0fdccba5c5353da960a1976358e3526bb678db21f7d2` and
the rebuilt object has SHA-256
`eb5aac33ef240e2133bcbd3c965d9da4708eb9883ebd264738db6a629c6a05f0`.
Independent read-only comparison confirms identical 23-name code-owner sets
and strict section equality for every function. The whole object hashes differ
only because the candidate also carries ordinary compiler metadata, COMMON
definitions, and three discardable pooled constants discussed below.

## Initialized-data boundary and ownership

The regenerated candidate's single `.data` section and the January csplit span
are both 236 bytes with flags `0xC0400040`, ten relocations, and normalized
SHA-256
`3a69a8b8a83037c3277fa86c09c7aa4e9b9da8b480e3facb87275548ee4c88b8`.
They now also agree on the external owner at offset 0 and static owner at
offset 224. This proves the corrected linked bytes, relocations, boundary, and
linkage; the earlier one-aggregate form remained unauthenticated source.

The natural split has independent support:

- bytes `0x00..0xDF` are 28 eight-byte records. Each record is a signed short
  meter id, two bytes of ordinary alignment padding, and a callback pointer;
  seven callback slots relocate to the seven sampler functions;
- bytes `0xE0..0xEB` are three string pointers for `"none"`, `"actions"`, and
  `"activation status"`;
- `_ai_profile_update` addresses the first region as an eight-byte-stride
  definition array, while `_ai_profile_change_render_spray` addresses only the
  second region as a pointer table;
- the independently reconstructed October executable uses the same bases,
  `0x2C8E98` and `0x2C8F78`, again separated by exactly `0xE0`;
- PDB2.00 labels `_global_ai_meter_definitions` at January RVA 2,847,832 and the
  next public datum at RVA 2,848,068, exactly 236 bytes later, but its type
  census has no AI meter/profile type and no private render-name label.

Thus PDB adjacency establishes the full occupied range but cannot authenticate
the rejected wrapper. A public 224-byte definition array followed by a private
12-byte semantic name table is the least speculative source model.

## BSS/COMMON and candidate-only runtime inventory

Three candidate COMMON definitions correspond to independently named January
globals:

- `_profilestring`, 2,048 bytes. PDB2.00 places the next named global exactly
  2,048 bytes later;
- `_global_ai_profile_string_position`, 2 bytes. January instructions use word
  loads/stores; the remaining gap before the next aligned global is linker
  placement, not part of this object;
- `_ai_profile`, 3,820 bytes (`0xEEC`). PDB2.00 places the next named global
  exactly 3,820 bytes later, and the January reset sizes prove a 12-byte header
  plus 28 136-byte meter records.

Csplit leaves these symbols undefined because it does not reconstruct their
COMMON ownership; the candidate sizes and source layouts restore the authentic
definitions rather than inventing new state.

The focused candidate has three additional selection-2
(`IMAGE_COMDAT_SELECT_ANY`) `.rdata` sections beyond those selected into the
January `ai_profile.obj` split:

- `"none\0"`, 5 bytes;
- `"actions\0"`, 8 bytes; and
- the 4-byte `0.05f` constant, owner `__real@3d4ccccd`.

The first two are required by the January initialized-data relocations, which
name their exact decorated literals but leave their definitions outside this
split. The float is an ordinary discardable duplicate of the exact selected
January owner in `source/ai/action_charge.obj` (same payload, flags, selection,
and normalized hash
`0b9d79ff0fc4d0a77ef398763bbe45c445501e826219f2b6a1aa709411ab631d`).
These records are natural compiler pooling, not new target-data credit. The
candidate's remaining runtime strings are the same target-owned assertion,
format, and diagnostic records. `.drectve` and `.debug$*` are compiler metadata.

No candidate-only code symbol is emitted. In particular, there is no
`point_from_line3d`, projection helper, inline-math body, or unrelated COMDAT.

## Source and house-rule review

- The `ai_meter` layout follows the observed 136-byte stride and signed-width
  loads. The 28-entry enum indexes the measured meter offsets, and each render
  row uses the corresponding named constant rather than magic switch/table
  indexes.
- `ai_profile_update` preserves the shipped January/October history bug: it
  subtracts an evicted sample but does not add the replacement to
  `history_sum`. This is explicitly documented rather than silently
  "corrected." Its signed bounds, modulo-60 update, count clamp, and average
  calculation agree with the two binaries.
- The seven sample callbacks use real subsystem data counts and the owning
  actor iterator API. Unit and swarm-actor totals are derived from named actor
  fields, not raw offsets or representation casts.
- The row functions use the exact January text, including the malformed actor
  row, and named meter/capacity constants wherever a project constant exists.
  The literal prop capacity 768 agrees with the owning prop source, which has
  no public maximum constant.
- The spray renderer's mode switch uses the local enum, keeps the target's
  active-only topology, obtains actors and units through typed access macros,
  and performs the three transparent camera-offset calculations directly.
  It emits no forbidden line helper and does not use inline assembly, forced
  inline/noinline, volatile scheduling, raw addresses, or type punning.
- Public prototypes added by this reconstruction live in `ai_profile.h`.
  Private prototypes and definitions remain `static` in the owning `.c`.
  Every parameter is on its own declaration line, no-argument lists use
  `void`, project scalar types such as `real` and `boolean` are used, and every
  function ends in an explicit return.
- `tools/fake_match_scan.py` reports zero findings across the C and header.
  Manual review found no inert branch, self-assignment, optimizer hack,
  nonsensical coincidental match, or fake dependency.

The actor iterator header intentionally exposes the API and a forward
declaration; this TU's 28-byte concrete view agrees with the canonical owner
and January caller frames. A shared type migration would affect several C2
consumers and is not required to authenticate this object's behavior.

This review also found a pre-existing cross-TU prototype violation in
`source/hs/hs.c`: it locally declared `ai_profile_change_render_spray` instead
of including the owning header. The pending integration now includes
`ai/ai_profile.h` there and removes the duplicate declaration. That HS change
is byte-neutral in the orchestrator's focused check and closes the
repository-level house-rule issue without moving the prototype out of its
owner.

## Admission disposition

The pending configuration correctly changes the existing unit status from
`NonMatching` to `Matching`:

```json
{ "name": "source/ai/ai_profile.c", "index": 434, "status": "Matching" },
```

It also adds one `config/semantic_data_matches.json` record for
`_global_ai_meter_definitions`, covering the complete 236-byte section, all ten
relocations, and normalized SHA-256 above. That explicit manifest is needed
because ordinary objdiff attributes the local strings differently; it is not a
byte or target exception. No semantic-function record is needed for the 23
ordinary exact functions.

Do not restore the rejected aggregate wrapper, suppress the natural pooled
constants, or claim the three discardable candidate constants as new data
bytes. Regenerate object-diff/treemap products through the normal workflow
rather than editing generated reports manually.
