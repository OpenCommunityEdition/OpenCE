# Fable Decals four-function intake (2026-09-06)

## Scope and snapshot

Read-only intake of the four Decals functions claimed by Fable commit
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e` against canonical
`b90402d960a75d312f2a3895c43850e9cca4ccb8`.

- Canonical `source/effects/decals.c` blob: `cfcb9b714b680fce706d4ba60d8c92c244b631ea`.
- Fable committed `source/effects/decals.c` blob: `d6c6c589fd6209de9c0547716d8e3506d0ec7bd7`.
- Both trees have the same committed `source/effects/decals.h` blob:
  `69cfb5a123ea1c68a02812e0f7a9ef0cb373c39a`.
- Fable facts/report SHA-256:
  `42d41924fe467b92ad4a437ca585c2a805f916d5ccd2153596e4f8e5a8088fb7` /
  `65e93e43dfd4430c6456c7ea281344864de626af788c3177b1298303ab771988`.
- Frozen Fable target/base objects SHA-256:
  `2c5c039c8aa9696171f5350b0f4f9826276114d90d366dd919dc0dfbc5cc6364` /
  `8b45aa02997648f53681cf6ce5f3fd8f568cc04391ef8c43dfe03c352de3c985`.

There is no dirty Fable Decals C or H diff. Its dirty `symbols.json` has no
Decals hunk, so the Decals evidence below is the committed packet, not a later
scratch mutation. No production, donor, build, or configuration file was
changed by this intake.

## Identity and current status

Canonical currently reports 15/33 exact Decals functions. None of the four
sections below exists in its rebuilt base object, so all four are distinct new
claims rather than already-admitted work. Fable's regenerated target and base
section records are strict equal for each function.

| Function | Meaningful / padded | Relocs | Normalized SHA-256 | Canonical |
| --- | ---: | ---: | --- | --- |
| `decals_initialize` | 155 / 160 | 18 | `a405de875be53c2f05c1b5bb2bf96a62d0d4d747422bdbcb7eec1a8a740757ab` | unwritten |
| `decals_initialize_for_new_map` | 165 / 176 | 18 | `a3d89d395e8da6ff92b52efc9c9b325bddfdec835c5b1ec1f4d3db76e59d3f97` | unwritten |
| `decals_unlock` | 364 / 368 | 29 | `bd2b7cb08fd653e4fd630a88d731ee8823d3c94e1e5daec9eb2d548feeb09927` | unwritten |
| `decal_delete` | 401 / 416 | 36 | `4f7d8baf5c19b26d74d14dcccffc9fab4783bbfa555b1a4313b140d998624860` | unwritten |

Total: 1,085 meaningful / 1,120 padded bytes. The Fable report records the
four bodies as first-compile exact modulo the BSS names that its committed
configuration then split. Its later Decals scratch variants concern the held
debug-rendering work, not retries of these four bodies.

## Ownership and semantic evidence

All four have independent HCEA semantic reconstructions. January remains the
authority for its DEBUG assertions, one-shot error latches, and geometry-count
resets, which the later Xbox/Release bodies do not all retain.

### `decal_delete`

This is independently owner-closed except for two private one-byte owners.
The existing `effects/decals.h` already declares its public `long`-handle ABI,
and `rasterizer_xbox_decals.c` is a genuine caller. Its body uses only current
owners:

- the complete current `decal_datum`, decal flags, `DECAL_GET`, and
  `global_decal_data`;
- current strict-exact `decal_get_first_decal_index` and TU-private
  `decal_set_first_decal_index`;
- `datum_get`, `datum_delete`, `error`, and assertion owners already included;
- the authenticated current `decal_globals` record/pointer.

January's 36 relocations confirm that graph. They also prove repeated
read/write ownership at aggregate Decals BSS offsets `+30928` and `+30929`
(absolute `0x43D580` and `0x43D581`). Fable splits those as the TU-private
booleans `decal_delete_locked_reported` and
`decal_delete_permanent_reported`. Their addresses and one-shot meanings are
directly proven; those descriptive spellings are inferred rather than
Release-PDB-attested and must be recorded as semantic split names, not claimed
original identifier recovery.

The natural behavior is complete: assert the datum, report locked/permanent
deletion once, unlink both neighbor directions, update either the disconnected
head or the per-layer/per-cluster head with the matching January assertions,
then delete the datum. It introduces no private function ABI, foreign
prototype, geometry owner, or lifecycle API move.

### Initializer pair

`decals_initialize` and `decals_initialize_for_new_map` are a coherent later
pair, but are not header-closed today. They require:

- public prototypes added to their real owner `effects/decals.h`;
- direct `saved games/game_state.h` inclusion for `game_state_data_new` and
  `game_state_malloc` (initialize only);
- existing `rasterizer/rasterizer.h` APIs for the two rasterizer lifecycle
  calls, without revisiting the rejected bitmap-lifecycle declaration move;
- the complete January TU-private geometry owner and target split name
  `_decal_geometry` at aggregate BSS `+200` / absolute `0x435D78`.

The asserted pool/global allocation and reset semantics are genuine. The
owner burden is nevertheless larger than the two bodies: their count resets
touch `decal_vertex_count` at geometry `+0x6000` and
`decal_surface_count` at `+0x6802`, so a prefix/opaque geometry facade is not
admissible. Fable proposes a January-specific 0x7804 record with 24-byte
vertices and a trailing 1024-entry surface-index array. The retained
Xbox/Release-PDB geometry is instead 0x5804 with 20-byte vertices and no
surface-index tail. That is plausible platform/version divergence, but the
full January record must be authenticated from all of its January consumers
before it is treated as a complete owner.

### `decals_unlock`

The public prototype already exists and HCEA corroborates its iterator/flag/
count semantics. Its January body additionally needs the complete geometry
owner above and two genuine one-shot private booleans at aggregate BSS
`+30926` / `+30927` (absolute `0x43D57E` / `0x43D57F`), split by Fable as
`decals_unlock_locked_count_reported` and
`decals_unlock_permanent_count_reported`. Those names are likewise semantic,
not PDB-attested. Because it resets both geometry counters, it should remain
withheld until that full owner is admissible.

## Chronology and previous boundaries

January's code order is:

1. `decals_initialize`;
2. `decals_initialize_for_new_map`;
3. `decals_dispose_from_old_map`;
4. `decals_dispose`;
5. `decals_unlock`;
6. later, after `decals_update` and
   `decals_delete_permanent_from_cluster`, `decal_delete`.

Fable placed all four new definitions contiguously at the start of the public
region. Therefore its exact records prove the bodies and named relocation
owners in that donor context; they do not prove that source order or current
canonical TU context is codegen-neutral. A canonical first-natural candidate
should use the semantic January neighborhood and must not retry positions or
declaration arrangements merely for shape.

Two earlier boundaries remain controlling:

- The first-index candidate once matched exactly but perturbed four inherited
  Decals sections through TU topology; it was restored and was only admitted
  later as part of a separately proved larger source transition. This is
  direct evidence that an isolated exact COMDAT is insufficient here.
- The 2026-09-05 rasterizer-bitmap lifecycle-owner header trial changed
  `decals_delete_permanent_from_cluster` and was fully restored. None of these
  four bodies requires retrying that three-header/source declaration move, so
  it stays excluded.

## Recommended next packet

**First-natural trial: `decal_delete` only (416 padded bytes).**

The coherent mutation boundary would be exactly:

1. add the ordinary public body at its January semantic neighborhood after
   `decals_delete_permanent_from_cluster`;
2. add only the two TU-private one-shot boolean definitions needed by that
   body;
3. add only their two exact-address semantic BSS split names to
   `config/symbols.json` and regenerate the target metadata;
4. make no header, geometry, rasterizer-lifecycle, or unrelated Decals change.

This is a trial recommendation, not an admission or predicted exact result.
It needs the normal whole-TU and repository-wide inherited-section sweep,
with special attention to `decals_delete_permanent_from_cluster`, plus a
bounded linked-list/latch runtime differential before acceptance. No body
spelling, source-position, qualifier, or local-lifetime variants should
follow a nonmatch without new source evidence.

**HOLD** the 160+176 initializer pair and the 368-byte unlock body until the
complete January geometry owner and initializer header blast can be proved as
one coherent boundary. Do not import the full four-function/geometry batch as
the smallest packet.
