# Particle-system lifecycle pair independent review (2026-09-06)

## Ruling

`particle_system_delete` plus
`particle_systems_reconnect_to_structure_bsp` is a coherent, source-credible
496-padded-byte packet once the separately reviewed particle-system records
are owned by their real headers.  The committed Fable control flow can remain
unchanged.  The only source corrections required by the stronger owner model
are semantic type/name changes (`particle_type`, `ps_particle_datum`, and
`particle_system_delete`) plus ordinary owner includes/declarations.  No raw
offset, local record facade, forced emission, export, inline helper, or source
shape variant is warranted.

This review is source/object read-only.  It did not compile, gate, or modify
production, configuration, build output, or the donor.

## First-natural provenance and target identity

The bodies are committed at Fable commit
`f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`, source blob
`74ca2e87940e159f7c921cd4c590d52f8ccbb7a0`.  Its report explicitly records
both functions as exact on the first compile.  The earliest retained object,
`C:/halo-worktrees/fable-50k-small-families-20260904/scratch/w_ps_batch1.obj`
(SHA-256
`40a5f5a9116a8a5750bf76ca338671b671993e60beb4987bc6bce68ee642dc9c`),
still uses the raw helper name and independently proves the source chronology:

| function | target / first-natural size | relocs | normalized SHA-256 |
|---|---:|---:|---|
| `_code_0008dd80` / `particle_system_delete` | 176 / 176 | 9 / 9 | `f2c3884188b26d73b9d5a9d81de37b82c608b7db3db3038c2ef295266580b8b4` |
| `particle_systems_reconnect_to_structure_bsp` | 320 / 320 | 15 / 15 | `c630b0dfe3bfedc35c02a46b52dee34411bf25ac8c7c5f6f7a5aedceef400175` |

Both complete section records, including bytes, relocation offsets/types and
targets, flags, and selection, compare strictly.  The target object is
`build/split/source/effects/particle_systems.obj` (SHA-256
`a6c6b08ada7fe558c34455c623d8cf5249798f7e023b3cb3ddd4167dc3ac20ba`).
The target/census reproducer and result are
`scratch/particle_system_lifecycle_pair_target_audit_20260906.py` and `.json`.

The donor later renamed the helper and its config metadata without changing
either body.  The final donor object is
`C:/halo-worktrees/fable-50k-small-families-20260904/scratch/w_particle_systems.obj`
(SHA-256
`1fbaece7e9e86fbf9d8d727ed85be28a5760072aae4c0627b7bd2f4f421a89dc`).

## January semantics and ABI

`particle_system_delete` is a one-argument cdecl helper: all three January
callers push one 32-bit datum handle, clean the stack as callers, and ignore
volatile EAX.  The body gets the system and definition, iterates the signed
`short` type index, saves each particle's next handle before deleting it,
then deletes the system datum.  Its nine relocations, in order, are
`particle_systems`, `datum_get`, `tag_get`, `system_particles`, `datum_get`,
`system_particles`, `datum_delete`, `particle_systems`, and `datum_delete`.

The public reconnect function is cdecl `void(void)`.  It iterates systems with
`data_next_index`; attached systems refresh location through
`object_get_location`, while unattached systems derive it with
`scenario_location_from_point` and delete systems outside every cluster.  It
then walks each type's linked particle list, refreshes particle locations, and
splices particles outside every cluster.  Its 15 relocations occur at target
offsets `+7,+16,+50,+57,+73,+99,+148,+155,+170,+187,+194,+217,+233,+279,+286`
and name only the expected two data arrays, datum/tag/location APIs, and the
same-TU delete helper.

Two unusual orders are authenticated January behavior and remain unchanged:

- after deleting an out-of-cluster particle, reconnect reads
  `next_particle_index` through the same datum pointer and then splices it;
- after deleting an out-of-cluster system, the outer iterator advances using
  that system handle.

The proposed source records the first ordering rather than silently moving
the load.  Neither order is claimed as a newly proven defect.  The type loop
also relies on the valid-definition domain (`types.count <= 4`); the packet
must not invent a clamp or assertion.

## Linkage and real retention

A relocation census over all 833 January split objects found exactly three
incoming calls to the raw delete helper, all in `particle_systems.obj`:

1. `particle_systems_reconnect_to_structure_bsp`;
2. existing exact `particle_systems_dispose_from_old_map`; and
3. the still-unwritten same-TU update function `_code_0008e7f0`.

The public reconnect function has one January incoming data relocation, from
the scenario structure-BSP reconnect procedure table.  The current canonical
source has that genuine table entry already.  Consequently the private helper
is naturally retained by real callers even before the unwritten update body;
no anchor or false export is involved.

The earliest natural object emits `_code_0008dd80` with COFF storage 3
(static), while the generic January split symbol has storage 2.  Same-TU-only
references and the recovered source topology support the proposed
`static particle_system_delete`, but do **not** constitute direct recovery of
the original PC linkage from January metadata.  The matching config rename is
therefore an explicit semantic/static inference:

`{ "file_offset": 580992, "flags": 32, "name": "_particle_system_delete", "static": true }`

## Owner boundary and exact patch scope

The packet must wait for the independently reviewed shared schema.  It needs:

- `particle_system_datum` size `0x158`, with four `particle_type` records at
  `+0x58`;
- `particle_type` size `0x40`, with `first_particle_index` at `+0x3c`;
- `ps_particle_datum` size `0x80`, with `next_particle_index +0x04`,
  `location +0x14`, and `position +0x1c`;
- complete `particle_system_definition` size `0x68`, with `types` at `+0x5c`;
  and
- `ps_particle_get` returning `struct ps_particle_datum *`.

The two bodies otherwise use existing ordinary owners only:
`datum_get`, `datum_delete`, `data_next_index`, `tag_get`,
`object_get_location`, and `scenario_location_from_point`.  They introduce no
literal, initialized data, BSS, COMMON, math helper, or point helper owner.

The narrow ownership cleanup is:

- include `scenario/scenario.h` in `particle_systems.c`;
- add the reconnect declaration to `particle_systems.h`;
- include that owner in `scenario.c` and remove its local reconnect and
  disconnect declarations;
- rename the existing dispose caller to the private semantic helper; and
- apply only the semantic/static config entry above.

The exact source fragment and machine-readable edit plan are
`scratch/particle_system_lifecycle_pair_proposal_20260906.c` and
`scratch/particle_system_lifecycle_pair_manifest_20260906.json`.

Excluded from this packet are `particle_system_initialize`,
`particle_system_new_unattached`, `particle_system_new_particle_explosion`,
all old expression/declaration-order experiments, and every forced-retention
device.  Their previously adjudicated boundaries are unchanged.

## Corroboration limits

Later HCEA binary reconstructions corroborate the semantic names and body
logic but are not original-source authority:

- `research-cache/halocea-full-blobs-20260830/src/blam/effects/particle_system_delete.c`
  SHA-256
  `1669394de0273413fd97a1b6d5fa4f25b7498acfc5d5ad0436c783ec814447bb`;
- `research-cache/halocea-full-blobs-20260830/src/blam/effects/particle_systems_reconnect_to_structure_bsp.c`
  SHA-256
  `55ba8631dc5a4103f434e0eeb67e90b1fb9ae18231a94c8227f674f91c7ab432`.

The authoritative claims here come from January instructions/relocations,
the full caller census, the committed Fable chronology, and the retained
first-natural object—not from those later reconstructions alone.
