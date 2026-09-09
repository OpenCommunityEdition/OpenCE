# Decals delete-latch reconciliation (2026-09-09)

## Scope

This packet admits only `decal_delete` from the previously audited Fable
Decals claim.  It deliberately leaves the initializer pair and
`decals_unlock` on hold because those functions require the unresolved full
January `decal_geometry` owner.

The starting canonical commit is
`6f8644c0cc7b7748f98b9a4624e89678ff5263c4`.  The prior read-only intake is
`docs/object_matching_logs/decals_obj_fable_four_claim_intake_20260906.md`.

## Source and ownership ruling

The public prototype was already present in `effects/decals.h`, and the body
was placed in its January neighborhood immediately after
`decals_delete_permanent_from_cluster`.  It uses the existing `DECAL_GET`
typed-access macro, semantic decal flag constants, existing list-head accessors,
and the true `global_decal_data` owner.  No foreign prototype or header change
was introduced.

January proves one-byte private storage at file offsets `4445568` and
`4445569`.  They are split as the TU-private semantic names
`decal_delete_locked_reported` and `decal_delete_permanent_reported`.  The
addresses and one-shot latch meanings are exact evidence; the spellings are
descriptive campaign names and are not represented as PDB-attested originals.

The recovered behavior is coherent rather than match-shaped: it diagnoses the
first attempted deletion of a locked or permanent decal, unlinks both neighbor
directions, repairs either the disconnected-list head or the cluster/layer
head, and finally deletes the datum.  The two January assertions guard the
datum and head ownership.  The implementation contains no address-derived
identifier, raw typed-access cast, manual flag mask, forced inline, undefined
behavior, or unrelated source perturbation.

## Exact result

The isolated whole-TU campaign gate reports:

- `decal_delete`: **401 meaningful / 416 padded bytes**, strict exact;
- Decals: **21 exact, 4 residual, 8 unwritten** of 33 functions;
- zero inherited Decals losses;
- no emitted `_point_from_line3d` symbol.

`tools/fake_match_scan.py source/effects/decals.c` reports zero review leads.
The exact section has the 36 January relocation identities recorded by the
prior intake, so the linked-list mutations, diagnostics, latch storage, and
datum deletion all retain their genuine owners.  This packet claims no credit
for the remaining residual or unwritten functions.

The repository-wide regression sweep and aggregate semantic accounting are
deferred to the containing >=10,000-meaningful-byte publication batch.  This
isolated result is not published independently.
