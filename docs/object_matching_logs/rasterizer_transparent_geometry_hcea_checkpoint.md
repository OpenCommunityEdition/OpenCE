# rasterizer_transparent_geometry.obj HCEA checkpoint

## Scope and provenance

This checkpoint advances the existing honest partial reconstruction without
changing its `NonMatching` status. January target code fixes the accessed
offsets and branch order; HCEA type evidence supplies the missing semantic
identities for the group geometry flags, source object, shader, depth-sort
value, and Cortana tie-break flag. The recovered fields are represented by a
typed 0xA0-byte translation-unit-local structure with compile-time size and
offset checks. The existing 0x4A-byte state owner is likewise unchanged in
size and has explicit offset checks for every field used by this cluster.

Only `source/rasterizer/rasterizer_transparent_geometry.c` and this new
Jonas/Codex ledger belong to the checkpoint. No shared header, config entry,
park, frozen-object path, existing Markdown, or Claude-authored Markdown is
changed.

## Strict code result

The forced XDK 3911 build preserves every one of the 13 previously accepted
functions and adds two strict semantic-COFF matches:

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_00173f70` | 37 | 48 | 4 | `da087f20466ace5d9fc4068aa6ea155a055bd6025b9b8a7f7cadd6a3e580a6c4` |
| `_code_00173fa0` | 374 | 384 | 13 | `ca47b94f27b1d623a785719222f959c25585bffeadd3bb3e6981a39693d2f475` |

The object therefore moves from 13/19 to 15/19 strict functions, from 818 to
1,229 meaningful exact bytes, from 896 to 1,328 padded exact bytes, and from
61 to 78 exact relocations. The delta is +2 functions, +411 meaningful bytes,
+432 padded bytes, and +17 relocations. All 15 accepted sections have exact
normalized bytes, relocation addresses/types, symbolic destinations, and
addends.

`_code_00174120` is retained only as the real, readable sort caller needed for
VC7 to emit the source-private exact `_code_00173f70` helper. It receives zero
credit: target and candidate are both 176 padded bytes with 14 relocations, but
their normalized hashes differ (`4f78b847...` versus `30cd227e...`). It is not
an anchor or artificial reference; it performs the target operation and is the
natural caller of both recovered functions. The remaining three target bodies
(`initialize`, `next_group`, and `draw`) stay absent.

The target split presents `_code_00173f70` as storage class 2 while the natural
source-private helper is storage class 3. Its code section and all four
relocations are strict exact, but this checkpoint makes no whole-symbol
ownership claim for that helper.

## Runtime data and ownership

Six of the target's eleven RDATA COMDATs are now exact in identity, flags,
selection, bytes, symbols, and relocations: 481 logical bytes total. The two
new comparator assertion strings account for 174 of those bytes. The natural
six-byte `"group"` assertion COMDAT remains candidate-only because the target
split represents that same-name relocation as an undefined cross-TU symbol;
it receives no data credit.

The existing 74-byte state aggregate remains the sole candidate COMMON owner,
with unchanged name, size, field offsets, and code relocation addends. The
target split represents it as a defined BSS section, so no hardened
whole-section ownership claim is made. This checkpoint adds no semantic-data
exception.

## Policy boundary

- the qsort comparator has the standard `int __cdecl(void const *, void const *)`
  signature and reads actual `short` array elements through const typed views;
- all group accesses use named fields with target-backed compile-time offsets;
- the target's unordered floating comparison is expressed with ordinary,
  defined C (`!(left < right)`), without bit tricks or representation punning;
- there is no new asm, volatile scheduling, forceinline, optimizer pragma,
  barrier, raw address reconstruction, incompatible pointer cast, inactive
  union read, byte forcing, or synthetic emission anchor;
- every added definition uses the campaign's parameter and explicit-return
  format.

## Gates

- forced XDK object rebuild: pass, warning-free;
- clean-commit object deletion/rebuild and 15-function strict replay: pass;
- strict whole-emitted-function comparison: 15 exact, one documented
  zero-credit residual, three absent, no loss;
- target-owned RDATA ownership audit: 6/11 sections and 481 bytes exact;
- full Halo and libcmt build graphs: pass;
- config remains `NonMatching`.

Aggregate semantic/admission/park/test results are recorded after integration,
where the already-hydrated reporting toolchain is authoritative.
