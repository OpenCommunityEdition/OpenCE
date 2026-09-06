# Observer: genuine shared schema, canonical verification (2026-09-05)

## Accepted header packet, not body admission

The canonical packet makes three narrow source changes:

- `camera/observer.h` names the existing five command-parameter groups and
  adds an anonymous `real parameters[14]` view over the existing named fields.
  All field offsets, the five timer/flag slots and the 0x68 command size remain
  fixed and are asserted. The pattern already exists in the real math types.
- Move the existing `observer_time_flags` enum verbatim from `director.c` to
  its genuine shared owner, `observer.h`. No new terminal count is added.
- Recover the real `tag_block predicted_resources` at `structure_cluster+0x28`
  within the existing 0x68 serialized extent. Unknown surrounding bytes remain
  unknown; no unrelated later-game schema is imported.

January's scalar parameter loops, two adjacent five-short count tables, and
the postcheck call of `predicted_resources_precache(cluster+0x28)` establish
these real storage/API requirements. Read-only DB-backed HCEA reconstruction
at `../../research/halocea`, commit
`c89106c4964f2df3a98ef7b4035d6750372797cd`, corroborates names and topology;
it is not original January source. No CEA-only offset is accepted on trust.

The cluster's natural source alignment changes from the old placeholder's 2
to the genuine pointer-bearing block's 4. Its serialized size and all known
member offsets stay unchanged. No packing or compiler-option accommodation
is used. The canonical inventory found only pointer/element consumers, no
by-value or embedded instance requiring the placeholder alignment.

This admits no Observer gameplay body, data symbol remapping, unused constant,
whole-object Matching status, or new exact-byte credit.

## All 46 dependent objects verified

Read-only Ninja dependency inspection covers all 572 built outputs. There
are 26 Observer-header dependents and 25 structure-header dependents, with
five shared: 46 distinct outputs. Both immutable before/after directories
preserve the complete relative `source/...` paths under scratch.

The full all-section audit includes anonymous code/data/directives, symbols,
COMMON, offsets, type/storage, COMDAT selection/alignment, ordered relocations
and complete target metadata. It reports no added/lost runtime owner, new
definition transition, real runtime change, COMMON change or COFF runtime
header change. Observer itself has no runtime change. Across eleven other
TUs, 24 code-section records change only compiler-local label spellings; their
positions, storage/type, relocation addresses/addends and destinations are
unchanged. Director accounts for four of those records. Raw original records
remain in the evidence, rather than deleting owner metadata to force equality.

Root independently reran this audit and its nine synthetic negative checks.
The full canonical build and stable sweep preserve all 6,217 strict owners
out of 8,245, with no losses or gains. All 281 parks validate; the admission
audit has zero contradictions/revocations. All 288 tests and 26 subtests pass.
The separately applied ActionAlert/Particles owner corrections have their own
complete runtime reports; they are not disguised as header-only differences.

## Active-lane isolation

The active `fable/50k-small-families-20260904` tree remains uninspected and
untouched. None of its reserved implementation TUs occurs in this canonical
46-output dependency union. This does not claim its unpublished header work
is unaffected. On release of that lane, reconcile exactly the canonical
Observer union/enums and cluster +0x28 field, preserving any independently
authenticated additional Fable schema. Do not overwrite either header
wholesale. AI Debug and Vehicles were verified only as existing dependents;
no implementation work was performed on them.

## Reproducible evidence

- `scratch/observer_schema_independent_review_20260905.md`: full January
  instructions, exact offsets, source provenance and dependency inventory.
- `scratch/observer_schema_dependency_review_20260905.json`:
  SHA `7e5274d2d7796963329fbb86e076387c910e3ed4471c1752e3b8d0b5f0690417`.
- `scratch/observer_schema_target_review_20260905.json`:
  SHA `3276d7fd0bd012f4f80c4e1da3c12e9cd1d01b6390efc7d60f9fee9a9451d6d8`.
- `scratch/observer_dependents_runtime_audit_20260905.py`:
  SHA `76d1978e49e62cfd7b6734ee578644729857e92158c533036b8777ab7b783f50`.
- Root output: `scratch/observer-runtime-root-review-20260905/summary.json`
  and its complete per-unit raw records.
- Frozen inputs: `scratch/observer-schema-canonical-before-20260905/` and
  `scratch/observer-schema-canonical-after-20260905/`.
- Final batch strict snapshot:
  `scratch/math-owner-schema-canonical-first-20260905.json`, compared to
  `scratch/transport-init-canonical-first-20260905.json`.

The independent caller-cluster review remains separately conditional on
actual production body verification and ordinary helper-owner compatibility.
