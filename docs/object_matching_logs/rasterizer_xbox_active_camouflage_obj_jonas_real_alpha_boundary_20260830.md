# `rasterizer_xbox_active_camouflage.obj` real-alpha one-shot boundary (2026-08-30)

## Result

`_real_alpha_to_pixel32` remains unwritten.  The only production candidate
preserved the recovered bounds assertion and conversion semantics, but it did
not reproduce January's compiler packet:

| Packet | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January | `99 / 112` | `6` | `28913a006347a05e936678a44d230c0137795a8e427083b4ff0f1367ecd73596` |
| sole natural candidate | `86 / 96` | `8` | `a852bddee9ed3847e45e2352a2acee95f3c97731944e16c7dc2b37cd65bf33bf` |

The candidate was removed wholesale without an expression, local-variable,
cast, declaration-order, compiler-flag, or source-location retry.  Production
source is byte-identical to the pre-shot committed blob
`6ed85c6cd714b931e2e4ca98a8e1fbc8efc1069b`.  The unit therefore keeps its
five inherited exact functions and six-byte BSS owner, adds no exact credit,
and remains `NonMatching`.

## Authority, history, and one-shot scope

January's split object is
`build/split/source/rasterizer/xbox/rasterizer_xbox_active_camouflage.obj`,
raw SHA-256
`c54a305b551a8913d7eeb75b29e8ed1b7c097ed5fd9392c443f273dcec036a70`.
Its `_real_alpha_to_pixel32` packet proves:

- two ordered `[0.0f, 1.0f]` comparisons;
- the original `alpha>=0.0f && alpha<=1.0f` assertion text and
  `..\\bitmaps\\bitmaps_inlines.h:0x123` provenance;
- calls to `display_assert` and `system_exit(-1)` on failure;
- a stack-materialized `255.0f`, x87 multiply, direct `FISTP` to a 32-bit
  local, memory left shift by 24, and return of the resulting top alpha byte.

The semantic donor is the clean HCEA checkout at
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`.  Its
`src/blam/effects/real_alpha_to_pixel32.c` blob is
`9ada221b9362894c50d8b8f25a33267524974af6`, payload SHA-256
`03bf85d1f1263627d97a2be4d588d4f7e81469e926d40c3e1bf9e4255adcbbd3`.
HCEA independently supplies the typed alpha-to-top-byte behavior; January is
authoritative for the PC assertion and conversion packet.  The authenticated
Stian tree at `fc11d861865735877d5b8b4c408b1a0c06dde3cf` documents the same
January inline provenance and direct-FISTP boundary in its
`rasterizer_xbox_decals.c` blob
`ef025965c874c4166c290d89c215eac3d94ac013` (payload SHA-256
`eab1ae5c39a31407e3c5675143c5f06f20bebf0edefe036c9da2f67b311af054`).

Before editing, the complete applicable active-camouflage ledger was read.
A source/history census across 1,524 registered worktrees, 1,487 refs, 9,853
reachable commits, the campaign Markdown/text records, canonical history, and
the registered research trees found no prior definition, candidate compile,
or fixed-boundary record for this function.  The four custom-ABI callback
parks and the exact visibility/BSS wave were explicitly preserved.

## Frozen candidate and measured compiler boundary

Starting from clean commit
`a6dd17fc06a7ba9c9b260545d437b02db2938697`, the sole natural candidate used
the canonical `pixel32` type, one `float` parameter, the established
`match_assert` facility with January's authenticated header provenance, and a
defined unsigned shift of the converted alpha byte.  It added no raw address,
raw offset, pointer/integer conversion, pun, undefined behavior, assembly,
volatile/register qualifier, intrinsic, pragma, attribute, compiler barrier,
synthetic caller, optimizer option, comparator exception, or object patch.

The only production compile produced raw object SHA-256
`25d9fee546808ba7a5287a3ffa1c1ca3bf1aba94e893a90e1d8c436f2bab5f4a`.
Its assertion prefix is semantically and structurally concordant, but the
current frozen compiler edge lowers `(long)(alpha * 255.0f)` through an
external `__ftol2` call and a pooled `255.0f` constant.  January instead
materializes the scale in the stack frame and emits a direct `FISTP`, with no
conversion-helper relocation.  The candidate consequently has two extra
relocations and is 16 padded bytes shorter.  This is a production-edge
compiler-lowering boundary, not evidence authorizing a source-shape search.

The clean pre-shot regression manifest is
`build/audit/rasterizer_xbox_active_camouflage_real_alpha_pre.json`, SHA-256
`b614947dfb3a7e380121818376dd30da7b5e0a644068e665ad8bf4a4c8e7b08f`.
The rejected object was retained only as ignored local audit evidence at
`build/audit/rasterizer_xbox_active_camouflage_real_alpha_candidate.obj`.

## Disposition

Do not repeat the HCEA-natural typed C body or tune expression topology to
chase the direct-FISTP packet.  Reopen only if authenticated January PC source
or production-edge evidence explains the missing conversion mode without an
object-specific compiler exception.  No production source, header,
configuration, semantic-match entry, parked entry, or protected subsystem is
changed by this ledger-only result.

## Restored-state validation

- The restored-object regression check reports all five inherited functions
  `still_exact`, no newly exact function, `changed_nonexact: []`, no warning,
  and no failure.  Direct hardened comparison independently reports
  `all_equal: true` for the same five packets.
- Complete `halobetacache_build`, `libcmt_build`, semantic, and progress gates
  pass.  Semantic audit scans 470 units, evaluates 4,921 functions, accepts
  4,814 exact functions, and reports zero unit errors.  Campaign progress is
  unchanged at 384/833 objects, 4,775/11,060 functions, and
  607,190/2,198,102 meaningful code bytes.
- Object admission is 0 candidates / 0 contradictions / 0 revocations.
  Park validation is 12 active / 0 stale / 0 invalid.  The complete tooling
  suite passes 212/212 tests.
- Protected Units sentinel `_unit_preprocess_node_orientations` remains exact
  at 1,920 padded bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `git diff --check` passes.  The final tracked diff contains this additive
  ledger only; the active-camouflage source blob is unchanged.
