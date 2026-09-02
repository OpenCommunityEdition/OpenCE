# `bitmap_group.obj` add-bitmap reconstruction (Codex, 2026-09-02)

## Outcome

`_bitmap_group_add_bitmap` is now present as a complete, typed, ordinary-C
reconstruction and is fuzzy-parked under the current house rule.  It advances
the unit from `5 exact / 0 residual / 1 unwritten` to
`5 exact / 1 parked residual / 0 unwritten`.  The five pre-existing functions
remain strict exact.

This is not a byte-exact claim.  January contains 995 meaningful bytes in a
1,008-byte padded section.  The retained function contains 989 meaningful
bytes in a 992-byte padded section, has the same count of 59 semantically
corresponding relocations, and scores 60.99099% in the generated objdiff
report.  Its normalized hash is
`2e0bbd84c45f240758cc76a2d4df8c78be689cfe9bc21af6a52e017f218e7ca0`;
January's is
`b5fee4632a47ed183bb1aec903552bd1b140a591e64d42625900c8b7bf8a7e69`.

## Evidence and retained behavior

The January COFF supplied the controlling disassembly, relocation topology,
four distinct validation diagnostics, assertion spellings and line anchors,
the 0x34-byte frame, and the 48-byte bitmap descriptor layout.  The later HCEA
implementation and the independently lifted PC routine were used only as
semantic corroboration.  They agree on the descriptor construction,
dimension and cube-map checks, format flags, two-stage resize, relocation of
existing pixel pointers, tight-packing check, append, and zero fill.

The retained implementation uses the semantic `bitmap_data` and
`bitmap_group` fields, `BITMAP_GROUP_TAG`, cseries flag macros, typed tag-block
access, the owner declaration for `bitmap_get_pixel_data_size`, named enum
constants, and `real`-project conventions.  It contains no raw structure
offsets, inline assembly, `volatile`, optimizer pragma, forced inline/noinline,
fake dependency, undefined behavior, or inert code-generation carrier.
January's redundant second cube/power-of-two guard is retained because the
target control flow proves it exists.

## Fixed point and prior work

The first material divergence remains the allocation of the simultaneous
16-bit type, depth, format, and bitmap-group-type lifetimes.  January keeps
type in DX and depth in CX before reusing DX for zero and group type; current
VC7 selects a different register schedule.  It then lays out the equivalent
diagnostic blocks in a different order.  The fail-closed classifier reports
`UNKNOWN`, so the manifest class is deliberately `unclassified`.

The earlier object ledger records the exhausted explicit-clear, flags-local,
entry-live-range, return-type, declaration-order, signature-store, predicate,
and branch-layout families.  That lane once measured a readable 1,008-byte,
59-relocation nonexact candidate, hash
`cb66dbd3530e77e72696fbc5ea73ada901568acc0aeda564f96d651918fabca0`,
but restored production and did not retain its source.  A fresh unreachable
blob and worktree scan found only the later HCEA reconstruction.  Repeating
score-only compiler shaping would therefore violate the current park-first
rule.

## Header-position regression caught

The natural public declaration was initially added to `bitmap_group.h`.
Full-tree verdict comparison showed that the otherwise unused declaration
changed C2 allocation in `_bitmap_2d_uncompress_from_mipmap`, reopening its
880-byte exact function.  Removing only that declaration restored it exactly;
the semantic field-name corrections remained inert.  The additional header
prototype is therefore not retained.  The external function definition is its
authoritative declaration until a real consumer requires an owner-header API.

## Validation

- `python tools/campaign/gate.py source/bitmaps/bitmap_group --all`:
  `5 exact / 1 residual / 0 unwritten` before applying parked accounting.
- Baseline-versus-final full-tree verdict map: `0` exact regressions.
- `_bitmap_2d_uncompress_from_mipmap`: restored to strict exact after the
  header-placement bisect.
- `python -m tools.parked_functions`: `91 active / 0 stale / 0 invalid`.
- Full `ninja all_source progress semantic_progress`: passed.
- `python tools/fake_match_scan.py --fail-on-findings` on the changed C/header:
  zero review leads.

Reopen only for authoritative January source or local records, recovery of the
historical readable candidate, a strict same-topology donor, or a compatible
compiler/QFE that naturally explains both the register and cold-block layout.
