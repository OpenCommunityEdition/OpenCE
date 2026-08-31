# `connected_geometry.obj`: add-triangle cluster no-UB boundary (2026-08-30)

## Outcome

The remaining add-triangle cluster was audited but not compiled.  Its three
owners form one private-ABI unit:

| Owner | Meaningful / padded bytes | Relocations | Target normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_000f2e20` | 189 / 192 | 8 | `c21c80fafbc4175df12584cdd83cf153b979272feeab86c9354405901df8253a` |
| `_code_000f2ee0` | 240 / 240 | 7 | `5fbffc0eaf45c2ee125c0ed7d8bf45b9bf4b376bb9fee8b20771c34bc6aec4ac` |
| `_connected_geometry_add_triangle` | 406 / 416 | 14 | `2207c1a6a0dc6790fb1dc54aeb46852933f3e0d6efb28bffeb65cf011d2c7958` |

The point helper and caller have strong ordinary-C donors, but the edge helper
contains a January-proven indeterminate automatic-byte read on the invalid
negative edge-count path.  Reproducing it in C would violate the campaign's
explicit no-undefined-behavior rule.  Initializing the byte or guarding the
path would be lawful but would add control/data flow absent from January.

No production source, header, configuration, comparator policy, or park entry
changed.  No code-producing compile was performed for this cluster.

## Provenance and private ABI

The readable Stian mappings are functionally certain from the constant address
delta and surrounding connected-geometry owners:

- `_code_000f2e20` / `FUN_00103600`, refined donor commit
  `b430a9e2fdb4802217c940a7b1cca4b49662e13f`;
- `_code_000f2ee0` / `FUN_001036c0`, donor commit
  `286b37080adb48696635f8b1ee01aa3fbb77b03e`;
- `_connected_geometry_add_triangle` / `FUN_00103860`, donor commit
  `dd51068816c122495dbeb35a4605acb542a27b06`.

January's point helper takes the point in EBX and the points array in ESI.  The
edge helper takes geometry in EAX plus three stack arguments.  These are
compiler-owned private conventions established by the same-TU caller, so the
helpers must not be shaped or admitted in isolation.  The caller's relocation
packet confirms three point-helper calls and one edge-helper call.

The point helper's eight relocations are three ordered getter/epsilon pairs,
then add/get: getter REL32 at `+0x15`, epsilon DIR32 at `+0x24`, getter at
`+0x3A`, epsilon at `+0x4B`, getter at `+0x61`, epsilon at `+0x72`, add at
`+0x8C`, and getter at `+0x9F`.

The edge helper's seven REL32 relocations are getter at `+0x1A`, add at
`+0x5B`, getter at `+0x72`, array-new at `+0x7C`, getter at `+0x9C`, add at
`+0xA4`, and getter at `+0xB5`.

## Defined-behavior blocker

At `_code_000f2ee0+0x03`, January uses `push ecx` only to reserve a four-byte
local.  The orientation byte at `[ebp-1]` is written only on:

- a forward edge match at `+0x47`;
- a reverse edge match at `+0x4D`;
- creation of a new edge at `+0x67`.

It is later read at `+0xBE`.  When `geometry->edges.count < 0`, the scan exits
through the initial signed `jle`, the unequal index/count test skips creation,
and the later append path reaches that read without any orientation write.
Stian's donor explicitly documents and preserves the uninitialized local.

Dynamic-array invariants make a negative live count invalid, but a runtime
precondition does not make an indeterminate automatic read defined C.  The
campaign rules prohibit relying on that path.  A default initializer would
emit a store missing from January; an explicit negative-count guard would add
a branch missing from January.  Neither is an exact-source hypothesis.

## Disposition and reopen criterion

Do not integrate `_connected_geometry_add_triangle` without both private
helpers, and do not compile the atomic cluster while the only authenticated
edge-helper topology requires undefined behavior.  The point-helper donor
remains research evidence rather than a separately admissible owner.

Reopen only with original Bungie source/local records, an independent lawful-C
donor that naturally proves the orientation byte defined on every path while
emitting January's packet, or an evidence-backed policy decision that changes
the repository-wide no-UB rule.  Binary-led initialization tricks, inline
assembly, `volatile`, `register`, pragmas, raw offsets, and object-byte patches
remain prohibited.
