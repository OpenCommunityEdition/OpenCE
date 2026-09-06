# Particles: restore the genuine point-validator owner (2026-09-05)

## Accepted source-only correction

Remove the local pre-header `valid_real_point3d` alias, its matching undef,
and the duplicate ordinary external body from `source/effects/particles.c`.
The existing `effects/particles.h` includes the true owning `math/real_math.h`,
whose inline API has the same coherent three-component validation logic.
The ordinary gameplay caller now uses that existing API directly. No header,
gameplay expression, prototype, compiler flag, inline control or source-shape
variant is introduced. The old wrapper dates to commit
`e6e3f56440c50403c265e93c76449dfe5cb85159`; the earlier particle reconstruction
and ownership ledger were reviewed before this one natural trial.

All 18 strict functions and both residuals survive. This is zero new exact
code/data credit and is not whole-object Matching. The separate existing
color-validator ownership is not silently normalized by this packet.

## Complete runtime and ordinary-link evidence

The one PID-isolated candidate changes precisely one runtime field:
`_valid_real_point3d` COMDAT selection changes from NODUPLICATES (1) to
SELECT_ANY (2). Every runtime byte, ordered relocation and target record,
symbol offset/storage/type, section flag, other section, and COMMON inventory
is unchanged. There is no new/lost runtime owner, definition transition or
COFF runtime-header change. Debug source/output-path records are separate.

The candidate's ownership matches ordinary math-header copies elsewhere.
In a complete 833-target/572-base census, Particles was the sole selection-1
copy among six canonical definitions of this API. Frozen full-object XDK
linker checks pair the reviewed Observer candidate with Particles, in both
orders, before and after the change. Both original pairs report LNK2005 for
the validator; neither corrected pair does. They still report 56 unrelated
unresolved symbols and produce no DLL. This is bounded duplicate-symbol
proof, not a claim of a complete executable link. No /FORCE, /IGNORE, stubs,
object patches or linker accommodations were used.

Root's actual Ninja object is runtime-identical to the candidate, including
the reviewed selection. Its baseline-to-actual full audit confirms exactly
the same sole change. No `_point_from_line3d` definition appears. The canonical
stable sweep remains 6,217/8,245 exact with zero losses, all 281 parks validate,
the admission audit has zero contradictions/revocations, and 288 tests plus
26 subtests pass. Only an inert pytest-cache permission warning remains.

## Frozen evidence

| Artifact (canonical-relative) | SHA-256 |
| --- | --- |
| `scratch/particles-math-owner-before-20260905.obj` | `3657f31d5da5b707c14f1439980266f0b9c13e31d16e066719ab7a1a13e3f34f` |
| `scratch/particles-math-owner-first-natural-20260905.obj` | `44be385d99b952507bb035dc8016688797c8d7d80b76909df1c05e06f7d46290` |
| `scratch/math-owner-schema-actual-particles-20260905.obj` | `bd089e80873d936e39ac8adb641fa146162756652c77a97a45f71ff6e868ef45` |

The source manifest is `scratch/particles_math_owner_natural_20260905.edits.json`.
Full candidate/link records are `scratch/particles_math_owner_audit_20260905.json`
and `scratch/particles_math_owner_link_audit_20260905.json`.
Root's independent actual-object report is
`scratch/math-owner-schema-actual-particles-20260905.json`, produced by
`scratch/math_owner_schema_actual_audit_20260905.py`. It retains complete raw
metadata; the selection transition is explicitly reviewed, not hidden by a
weaker comparator.
