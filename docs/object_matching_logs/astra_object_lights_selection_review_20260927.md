# Object lights: provider-selection review remains held

Canonical reviewed: `a8b2512d630b88fa219e09729bbdc7c76e23a516`.
Donor admission not applied: `6bfd934f5ac14ee1535d1f1a327f5e7d2d98f1e7`.
Zero source/config changes and zero credit from this review.

The owner permits the one `_object_get_bounding_sphere` exception only if its
sole differences are authenticated `__FILE__` targets and the intended link
demonstrably discards this copy, retaining a January-identical provider.
Two-order duplicate-definition checks alone are explicitly insufficient.

## What the donor actually established

RF-BG establishes January's provider and a reproducible selection mechanism.
Its controlled link orders Halo inputs by descending January module index;
the linker selects action_vehicle, while swapping the contested objects
selects object_lights. The selected bytes and file-literal references were
inspected, not inferred solely from linker success.

However, the full-input diagnostic image used `/FORCE:MULTIPLE` for two other
duplicate definitions and a synthetic stub object defining 472 unresolved
names. These qualifications appear in RF-BG's report/notes but are absent from
the proposed admission commit's description and semantic-data reason. That
image is a controlled selection experiment, not an unmodified production link.
No image was executed during this review.

## Current canonical checks

The independent read-only audit found all 65 January-owned nondebug sections
and 71 symbols exact in object_lights, including flags, relocation identities,
offsets and storage. All 99 nondebug sections and 104 definitions compared
against the donor proof object are unchanged.

The current checked-in `tools/link_probe.py` is an opt-in diagnostic, not a
default build/link recipe. It consumes manifest order, not RF-BG's constructed
descending order, and prohibits `/FORCE` and dummy functions. In that manifest
the earliest `_object_get_bounding_sphere` provider is vehicles (index 35),
whose copy is January-identical. The other seven non-object_lights providers
are also identical. Therefore, a different provider module name is **not** a
byte mismatch or a reason by itself to reject the exception.

The unclosed requirement is a verified current intended-link contract and its
selected output. The default build compiles/splits/reports objects and does not
establish that executable-link contract. Neither a constructed diagnostic order
nor hypothetical first-provider reasoning is reported as an executed final
link. The donor experiment remains useful evidence, not discarded work.

## Disposition and next action

Keep object_lights NonMatching and its existing function/data credit unchanged.
Do not import the donor status flip or remove the data entry's incomplete-unit
qualification yet. No other COMDAT policy changes.

Reopen after establishing the intended link recipe/order and verifying on
current inputs that a January-identical helper is selected, the exceptional
copy is discarded, and the resulting literal references agree. Any use of
forced duplicate resolution or unresolved-symbol stubs must be disclosed and
kept separate from production-link claims; a further owner decision may choose
to accept a suitably bounded diagnostic proof instead.

Independent receipts and exact comparisons are preserved in
`scratch/astra_object_closeout_20260927/object_lights/`. Donor RF-BG proof,
controls and limitations remain untouched in its worktree.
