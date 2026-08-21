# `bitmap_group.obj` Jonas policy-clean partial recovery

This ledger records the fail-closed four-function subset retained in
`source/bitmaps/bitmap_group.obj`. The object advances from 0/6 to 4/6
strict-exact functions and remains `NonMatching`. No tag-definition data or
whole-object credit is claimed.

## Retained boundary

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_00065210` | 3 | 16 | 0 | `1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707` |
| `_code_00065220` | 29 | 32 | 2 | `33c6547344b3c246a1ab1b716d9f08aa8215c50ed15859502f0bf9163bc56e12` |
| `_bitmap_group_try_and_get_bitmap` | 65 | 80 | 2 | `cdffd5f0725497f3ef402d962ea88690ecbdd4cb3a0472678f8b939f7073f3d6` |
| `_bitmap_group_get_bitmap_from_sequence` | 220 | 224 | 8 | `541d8c8cfb4fbc9a89d92b42d2e248284c95e3eb84c54703b3fa210058d29e16` |
| **Retained delta** | **317** | **352** | **12** | |

The source uses the existing typed `bitmap_group_get` accessor and
`TAG_BLOCK_GET_ELEMENT`. File-local 0x20 sprite and 0x40 sequence layouts are
size checked. The void delete callback ends in an explicit `return;`; all
parameters follow the project line-format rules.

## Fail-closed withdrawal

The first recovery lane also reproduced `_code_00065240` at strict byte and
relocation equality (1,041 meaningful / 1,056 padded / 47 relocations).
Independent policy review rejected that source. One call was written as an
explicit discarded `TAG_BLOCK_GET_ELEMENT` solely after observing January's
REL32 at function offset `+0xAD`. The available HCEA source donor contains no
corresponding successor access in that loop, and the preserved transcript is
the Codex reconstruction itself rather than independent source provenance.
That makes the call a target-derived synthetic relocation anchor under the
campaign's current rules. The complete `_code_00065240` body and all of its
support-only declarations/constants were therefore removed, despite its exact
machine code.

`_bitmap_group_add_bitmap` remains absent as well. Its previously measured
readable candidate was nonexact at the same 1,008-byte / 59-relocation shape.
No tuning or alternate codegen control is retained.

## Provenance and scope

- Integration base for the recovery wave:
  `7d2ff479dde012f94cc08857e4cc74650b85b9af`.
- January split object SHA-256:
  `0772fa142d63ce966c625c0c4416e61a4500ac316321f132d29c83b889cbeed1`.
- HCEA independently supplies the four retained bodies' names, signatures,
  control flow, and typed field intent. January COFF remains the sole byte,
  padding, relocation, destination, and addend authority.
- The 1,424-byte target `.data` tag-definition table, broader target `.rdata`
  inventory, and every associated owner remain absent and uncredited.
- Scope is this C file plus this new Jonas ledger. No shared header,
  configuration, frozen source, existing/Claude Markdown, or parked manifest
  is changed.

The retained subset contains no assembly, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, raw address dereference,
pointer pun, inactive-union access, undefined-behavior match, synthetic
anchor, or byte/code-generation forcing.
