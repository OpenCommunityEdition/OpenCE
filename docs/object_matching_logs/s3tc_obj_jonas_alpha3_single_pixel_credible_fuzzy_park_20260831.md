# `s3tc.obj` Alpha3 single-pixel credible fuzzy park

## Outcome

`_DecodeBlockAlpha3__single_pixel` is a newly reconstructed, donor-backed
ordinary-C leaf.  It is **not** strict exact and no strict code or object
credit is claimed.  The source is retained and fuzzy-parked because it is a
coherent typed implementation with the sole target call relocation. The
residual is deliberately left unclassified: the corrected source-width
evidence changes the tail and padded envelope, so calling it a pure register
allocation tie would overstate what is known.

| owner | January padded bytes / relocs / normalized SHA-256 | retained padded bytes / relocs / normalized SHA-256 | ordinary fuzzy |
| --- | --- | --- | ---: |
| `_DecodeBlockAlpha3__single_pixel` | `432 / 1 / b869ae2d8e5be4b6efe4fef80b120931dfc7da90d6d9c21a03a60b019f8e6ba5` | `448 / 1 / db57324424c861cd2aa46b18de4452ba9f44e124138cbc496e7884d616d70c79` | `94.13816%` |

The sole relocation is `IMAGE_REL_I386_REL32` at `+0x1D`, resolving to
`_DecodeBlockRGB__single_pixel` in both objects.  The inherited
`_DecodeBlockAlpha3` remains strict exact at 608 padded bytes, one relocation,
and normalized SHA-256
`0dd23ce088e93017f7424049431b394d4d44e2730f3e8b0835aaee58e79a6a0d`.

## Provenance and ABI

The strongest type evidence is the local Halo Symbol Atlas entry that maps
January's own RVA `0x60160` to an exact-equivalent typed original-Xbox symbol.
Windows DbgHelp demangles it as a `cdecl` routine taking a const Alpha3 block,
an output color, and `short, short` coordinates. Exact records from original
Xbox builds dated 2001-08-15, 2001-09-25, and 2001-09-26 independently give
the same const-input and short-coordinate types for the RGB and Alpha3
single-pixel helpers; no typed exact record uses long or mixed widths.

The local HaloCEA reference at commit `c168af2e`, including
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\halocea-reference\src\DecodeBlockAlpha3__single_pixel.c`
supplies the exact-name single-texel DXT5 algorithm: decode RGB first, build
the two endpoint-dependent eight-entry alpha ramp, select one of the two
24-bit alpha-selector halves by row, and write the selected alpha channel.
That repository describes itself as an LLM re-sourcing of the HCEA binary; it
is cross-build semantic evidence, not original Bungie or Microsoft source.
January independently confirms every substantive part of that shape:

- a four-argument `cdecl` call to `_DecodeBlockRGB__single_pixel` with the RGB
  subblock at `source + 8`, output color, `u`, and `v`;
- two byte endpoints widened into a stack-resident eight-`word` alpha table;
- signed divide-by-seven and divide-by-five ramp arithmetic;
- the `v < 2` half selection and `3 * (4 * v + u)` selector shift; and
- the final write to `color->rgba[S3TC_ALPHA]`.

The initial branch incorrectly inferred `long u, long v` from full-dword cdecl
stack loads. Four-byte argument slots do not prove 32-bit source types.
January compares this function's row as `cmp di, 2`; neighboring RGB and
Alpha4 single-pixel functions use `SI`, `BX`, `BL`, and `movsx ..., si`; and
the HCEA donor plus its callers consistently use signed 16-bit coordinates.
The owner-header declarations and retained Alpha3 definition therefore use
const input pointers and `short u, short v`. The prototype belongs in
`source/bitmaps/s3tc/s3tc.h`,
the translation unit's associated header, because the helper is
target-external and called from other bitmap code.

The retained body uses existing named `s3tc_block_alpha3`, `s3tc_block_rgb`,
and `s3tc_color` structures.  It adds no new global, data, BSS, raw address,
representation cast, byte pointer, asm, volatile/register hint, pragma,
barrier, undefined behavior, inert expression, or synthetic control-flow.

## Measured residual

The target and corrected candidate are instruction-identical through
`+0x165`, just after the alpha-ramp branch converges. The residual begins with
the selector accumulator setup and the corrected short promotions:

```text
January:  use full incoming slots for arithmetic; compare v in DI; build selector in EDX
retained: sign-extend u/v for arithmetic; compare v in DI; build selector in EAX
```

Both select the same bytes and use the same variable shift, mask, alpha-table
load, and channel store. The corrected build adds sign extensions that
January omits and also chooses a different selector accumulator/schedule. The
fail-closed blocker classifier reports `UNKNOWN / unclassified`; it does not
authorize a register-allocation recipe.

A bounded width grid was measured before retaining the source-backed form:

| `u` / `v` | Candidate bytes | Fuzzy | Decision |
|---|---:|---:|---|
| `long` / `long` | 432 | 93.677635% | rejected: contradicted by low-word family evidence |
| `long` / `short` | 448 | 94.13816% | rejected: donor/callers also make `u` 16-bit |
| `short` / `long` | 432 | 93.2829% | rejected: target compares `v` as a word |
| `short` / `short` | 448 | 94.13816% | retained: strongest source/type evidence |

This makes the policy choice explicit: the most plausible types win even
though another spelling happens to reproduce the target envelope.

This is parked rather than tuned.  Do not introduce pointer-punning,
artificial aliases, manually reordered byte tricks, dead locals, or fabricated
branches to choose target bytes. Reopen only with January local/source
provenance, authentic Microsoft-era S3TC source, or a demonstrated natural
source distinction that explains both the missing promotions and tail
allocation.

## Gates

- One focused XDK 3911 compile completed successfully.
- Hardened comparison preserves `_DecodeBlockAlpha3` strict exact and records
  the new leaf at 432/448 target/candidate bytes with one matching relocation.
- The pre-wave whole-TU regression manifest reports the inherited exact Alpha3
  owner as `still_exact`.  Its `SYMBOL_SET_CHANGED` failure is expected for
  the added, still-nonexact target function and is recorded rather than waived.
- `tools/bijection_walk.py` records 16 tail events beginning at `+0x165`; the
  fake-match scan reports no review leads, and the fail-closed classifier
  reports UNKNOWN.

The unit remains honestly `NonMatching`: one strict exact function, one
fuzzy-parked residual, sixteen target-only functions, and all 120 target-owned
non-code bytes unresolved.
