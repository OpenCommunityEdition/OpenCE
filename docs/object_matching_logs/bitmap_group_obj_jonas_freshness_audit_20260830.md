# `bitmap_group.obj` freshness audit (2026-08-30)

## Outcome

`source/bitmaps/bitmap_group.obj` remains `4/6` strict exact and cannot be
closed from the currently authenticated evidence.  The machine-wide census
found no exact `_bitmap_group_add_bitmap` artifact and no artifact with any of
the target's five external data owners exact.  No production source, header,
configuration, ownership, park, or comparison policy changed.

The honest residual packet is:

| Owner | January padded bytes | Relocations | Current state |
| --- | ---: | ---: | --- |
| `_code_00065240` | 1,056 | 47 | unwritten; historical source is byte exact but was withdrawn under the synthetic-anchor provenance rule |
| `_bitmap_group_add_bitmap` | 1,008 | 59 | unwritten; best historical readable candidate has the same envelope but a nonexact register/CFG schedule |
| external `.data` owner cluster | 1,424 | 152 | absent and uncredited |

The data cluster owns `_global_bitmap_reference`,
`_global_bitmap_reference_optional`, `_bitmap_pixel_data`,
`_color_plate_data`, and `_bitmap_group`.

## Fresh source and object census

The census covered all 1,577 registered worktrees, reachable Git history,
normal `build/base` objects, ignored audit/scratch/output artifacts, and the
available HCEA, Punpckhdq, Stian, Pastudan, Claude, and Fable material.

- 1,577 worktree source copies collapse to five content hashes.
- Only one active source hash defines `_code_00065240`; it is the already
  documented five-function recovery source.
- No active source hash defines `_bitmap_group_add_bitmap`.
- 17,994 candidate object paths collapse to 17,585 distinct raw COFF objects.
- Exactly three distinct artifacts contain strict-exact `_code_00065240`:
  the known atomic/five-function artifacts and one duplicate gate artifact.
- No artifact contains strict-exact `_bitmap_group_add_bitmap`.
- No artifact contains even one of the five external target data owners at
  strict section/owner/relocation equality.

The three helper artifacts are therefore repetitions of the known withdrawn
source, not an unintegrated whole-object donor.

## Claude/cross-build corroboration

The external Claude tree at
`C:\halo-worktrees\claude-ai-conversation-2\src\halo\bitmaps\bitmap_utilities.c`
contains a PC reconstruction of the postprocessor and add routine.  Its
postprocessor independently shows a lookahead
`tag_block_get_element(..., i + 1, 0x40)` in the sequence walk.  That is useful
cross-build corroboration for the otherwise controversial discarded access
which made the January helper exact.

It is not a whole-object completion premise.  The Claude body is a
disassembly-derived raw-offset reconstruction, does not supply the January
tag-definition table, and does not provide an exact XDK 3911 add-helper
artifact.  The natural typed HCEA add body and its major branch/lifetime
families were already compiled and measured in the prior atomic ledger.
Re-emitting the same semantics would repeat the documented fixed point.

## Reopen rule

Reopen only with a new source-backed topology that explains both January's
DX/CX early allocation and validation fallthrough in
`_bitmap_group_add_bitmap`, plus a typed reconstruction of the complete
1,424-byte/152-relocation tag-definition table.  A helper-only recovery cannot
close the object and is not retained by this whole-object lane.

The complete prior experiment matrix and decoded data map remain in:

- `docs/object_matching_logs/bitmap_group_obj.md`
- `docs/object_matching_logs/bitmap_group_obj_jonas_five_function_recovery.md`

