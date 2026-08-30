# `bitmap_utilities.obj` `bitmap_fade` first shot (2026-08-29)

## Result

One frozen, ordinary typed C89 candidate was compiled for the previously
unwritten `_bitmap_fade`.  It reproduced the 400-byte padded envelope and all
16 relocation destinations, but not the complete normalized bytes or ordered
relocation addresses.  The body and its support-only declarations were
inverse-reverted immediately.  This wave retains no production source and
claims no function, byte, data, or object credit.

| Evidence | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `387 / 400` | `16` | `9efdf6d44e1d8f303986cf3b6545ba61f10e8254a851ea300df0540bf1a3bb8f` |
| Frozen natural candidate | `389 / 400` | `16` | `c9fd61a209f47de9359495438d1e308dc5ecc9f60de44ce49ad5048de0623417` |

The January split object's raw SHA-256 is
`efb68953ba917a1a9e758d175b9778fefb638b43f31bdfece342dcbb02a0579e`.
The candidate object's raw SHA-256 at measurement is
`991e03b1e858db260339755b8a439e19bd8363d34a1d45fd59571dbcced6d112`.
Whole-object hashes include compiler debug metadata; strict packet evidence
is the normalized function plus ordered semantic relocations.

## Documentation and provenance

The campaign playbook, matching methodology, object-ledger README, and all
reachable `bitmap_utilities.obj` ledgers were checked before source work.  No
object ledger or ref-reachable source attempt names `_bitmap_fade`; reachable
history contains only the original symbol inventory for this body.  The
canonical repository contains no `CLAUDE.md` or `AGENTS.md`.

The Stian donor tree's complete `CLAUDE.md` and `AGENTS.md` were read.  They
have SHA-256 values
`116eda38343bf1a290f0b04797a74ab92c97dfa12eeb2a17f813480f5ec70bbb`
and
`a5188f5a573ac3dcf077f50f49ffb99cfb69b1df138f414242207e53dc975b3d`;
their only textual difference is the local `tool-reference` skill path.
Those documents govern a functional October retail reconstruction and do not
weaken this repository's January exact-byte gate.

Two independent public reconstructions agree on the fixed-point fade
semantics:

- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, exact-name source blob
  `38206ad8738b2f27caae74751fba8c925d8829c6`, payload SHA-256
  `ae6565f8527c4cd1098c9ebc3e41aca197b77e21f5ffd4e1cdee6b3b4ebe2792`;
- Stian commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, containing source blob
  `b93726998bc70650600b3d5eac028ab261c3ccb1`, payload SHA-256
  `eeccfd4bc2e7358c623970dc84eaff84d41c28388f30363a80e6f55c73dfd864`.

Both clamp a positive fade amount to `[0,1]`, round `amount * 256` with
`floor(x + 0.5)`, precompute four source-color channel weights, obtain the
base mipmap and pixel count through named APIs, and update every channel with
`(old * (256-alpha) + color * alpha + 127) >> 8`.  Stian additionally records
the authenticated `bitmap_verify(bitmap, TRUE)` assertion at line `0x1F5`.
January disassembly and relocations independently confirm that assertion,
the clamp constants, `_floor`/`__ftol2`, both bitmap helper calls, channel
precomputation, and the packed per-pixel loop.

The compiler is Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`,
SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
under the unchanged XDK 3911 production flags.

## Frozen candidate and boundary

The single candidate used `struct bitmap_data *`, `pixel32`, `real`, named
channel locals, public typed helper prototypes, C89 declarations, and an
explicit terminal return.  Its first `0x48` bytes agree with January.  The
first raw divergence is at `+0x48`, inside clamp evaluation.  January loads
`fade_amount` once for the zero comparison and then reloads it separately for
the lower-bound comparison.  The donor-shaped candidate keeps an extra x87
copy live across that comparison, producing a second `fld [ebp+0x10]` and
paired `fstp st(0)` cleanup instructions.

That different x87 lifetime shifts the target's second zero-literal
relocation from `+0x51` to candidate `+0x54`; later relocations remain the
same destinations but are shifted, and the candidate has two additional
meaningful bytes.  The common suffix and final epilogue converge again.
This is a source-expression/lifetime boundary, not a missing semantic call.

No alternate clamp macro/expression, declaration order, spelling, channel
order, or compiler-control form was attempted.  Do not retry the local
`clamped` donor transcription or infer a replacement expression from this
miss.  Reopen only with authenticated January source or independent same-
compiler provenance that predicts the target's repeated-load clamp topology.

The candidate used no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, optimizer barrier, raw address or byte offset, pointer/integer or
union pun, undefined behavior, synthetic anchor, object patch, compiler flag,
semantic exception, or comparator exception.  The final source Git blob is
restored to `c2cac170f88adfaeaf55b7f04eebd4628c96f6e5`.

## Validation

The restored production object rebuilds cleanly.  Direct hardened comparison
reports `all_equal: true` for all nine inherited exact packets:
`_bitmap_fill`, `_bitmap_alpha_to_rgb`, `_real_rgb_color_brightness`,
`_rgb_color_to_hsv_color`, `_real_rgb_color_to_real_hsv_color`,
`_argb_color_to_real_argb_color`, `_rgb_color_to_real_rgb_color`,
`_pixel32_to_real_argb_color`, and `_pixel32_to_real_rgb_color`.

The complete Ninja graph, report, semantic report, and progress pass.  The
semantic audit scans 470 units, evaluates 4,917 functions, reports 4,800
semantic-exact and 4,810 accepted-exact functions, and has zero unit errors.
Campaign progress remains 384/833 linked objects, 4,771/11,060 functions,
and 606,187/2,198,102 code bytes; Halo remains 282/468 linked objects,
4,604/7,574 functions, and 593,273/1,770,166 bytes.  Admission is
0 candidates / 0 contradictions / 0 revocations.  Parks are 12 active /
0 stale / 0 invalid.  All 212 tooling tests pass and `git diff --check` is
clean.  The regenerated report and semantic-report SHA-256 values are
respectively
`19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`
and
`f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.

The protected `_unit_preprocess_node_orientations` sentinel remains exact at
1,920 padded bytes, 87 relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
