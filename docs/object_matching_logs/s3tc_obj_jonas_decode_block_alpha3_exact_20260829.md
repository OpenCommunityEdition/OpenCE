# `s3tc.obj` `DecodeBlockAlpha3` exact recovery

## Result

`_DecodeBlockAlpha3` is strict semantic-COFF exact on its first and only
production candidate.  `s3tc.obj` advances from 0/18 to 1/18 exact functions
and from 0/6,880 to 608/6,880 strict padded code bytes.  The object remains
honestly `NonMatching` with 17 unwritten functions.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_DecodeBlockAlpha3` | `606 / 608` | `1` | `0dd23ce088e93017f7424049431b394d4d44e2730f3e8b0835aaee58e79a6a0d` |

Direct `tools/coff_compare.py` comparison reports `all_equal: true`.  The
sole `IMAGE_REL_I386_REL32` relocation is identical in both objects at owner
offset `+0x17`, resolving to `_DecodeBlockRGB+0`.  The January and retained
whole-object SHA-256 values are respectively
`95623e800d131322e6a079f07f78af41166864d8d3a29b7fe88ef788bc76a9dc`
and
`22b7821d8bab234a77a1a07cf264f8a97f9dcd97e94714604aa003a79d2fb042`.

## Authenticated source provenance

All applicable object ledgers, campaign methodology, audit tooling notes,
and Claude documentation were read before source work.  The current and
ref-reachable history contains only the empty initial `s3tc.c` skeleton for
this symbol, and no ledger mentions a previous `_DecodeBlockAlpha3`
production attempt.  The neighboring `_EncodeBlockAlpha3` first-shot no-go
was read and respected; this decoder is an independent function and does not
reuse or alter the rejected encoder.

Two public artifacts independently supply the same natural decoder:

- HCEA commit `c168af2e747d3095d9a29418ae401f3a39544863`, file
  `src/DecodeBlockAlpha3.c`, blob
  `7164dec1a7c18224d3eab516b0bfd00ebc55e800`, payload SHA-256
  `793a082d0da1f93d0e359ba56666b628a2e67da4a03f28cb56503e81e9379059`.
- Microsoft's preserved 1998 Direct3D Reference Rasterizer source at mirror
  commit `5c6fe3db626b63a384230a1aa6b92ac416b0765f`, file
  `multimedia/directx/dxg/ref8/common/dxtn.cpp`, blob
  `7a2508f7c313dc77aca8debc2936bf000a83edab`, raw payload SHA-256
  `cf656abba9123006face4966776eceb962ed1f9285b2c6e3c97ec789b4156f0a`.

The Microsoft body is especially strong provenance: it calls the RGB block
decoder first, builds the exact signed eight-entry alpha table, chooses the
six- or eight-alpha interpolation ramp from the endpoint comparison, loads
each three-byte bitmap half through sequential shifts and ORs, and decodes
all 16 three-bit selectors through one ordinary `for` loop.  January's
disassembly agrees instruction-for-instruction, including the signed
division-by-five and division-by-seven sequences and the otherwise surprising
four-pixel loop unroll.  That unroll is produced naturally by VC7 from the
plain Microsoft loop; no source unrolling or emission forcing was added.

The retained header owns named, asserted `s3tc_color`, `s3tc_block_rgb`, and
`s3tc_block_alpha3` layouts.  The alpha endpoints occupy bytes 0 and 1, the
six selector bytes occupy bytes 2 through 7, and the typed RGB block begins
at byte 8.  The retained C89 body uses those fields directly and preserves
the Microsoft statement, branch, and lvalue topology.  The retained source
and header Git blobs are respectively
`5a7a2015ff994a1462a8a7a0767adb5ca51fb879` and
`711c9a9342463681c4a314a1f9c436590399391f`.

The January compiler is Microsoft 32-bit C/C++ Optimizing Compiler
`13.00.9254.1`, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
under the unchanged XDK 3911 production command and flags.  The source image
SHA-256 is
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.

## Policy and validation

The implementation is readable typed C89 with one parameter per line and an
explicit terminal return.  It introduces no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, optimizer barrier, raw address, byte offset,
pointer or union pun, undefined behavior, compiler-option change, object
patch, semantic exception, comparator exception, or code-generation forcing.
The exact result came from one natural production compile; no spelling,
declaration, loop, type, or branch retry was performed.

The complete `halobetacache_build`, `libcmt_build`, report, semantic report,
and progress graph pass.  The semantic audit scans 470 units, evaluates 4,919
functions, finds 4,802 semantic-exact and 4,812 accepted-exact functions, and
reports zero unit errors.  Campaign progress is now 384/833 linked objects,
4,773/11,060 strict functions, and 606,876/2,198,102 strict code bytes; Halo
is 282/468 linked objects, 4,606/7,574 functions, and
593,962/1,770,166 bytes.  The strict board reports 277/619 fully exact
objects, 4,784/8,246 functions, and 663,783/1,922,669 padded bytes after
excluding parked and assembly-implemented credit.

Admission is 0 candidates / 0 contradictions / 0 revocations.  The parked
inventory is 12 active / 0 stale / 0 invalid.  All 212 tooling tests pass and
`git diff --check` is clean.  The regenerated report and semantic-report
SHA-256 values are respectively
`b13ef78fdc5dce935a82cdf93d76a474c3273dc54148975139f7c3114b2d43d0`
and
`51d605379dee1e1d44685c048d51150e13d504ae617a5ee1d1cb450d93fb431b`.

The protected Units sentinel `_unit_preprocess_node_orientations` remains
strict exact at 1,920 padded bytes, 87 ordered relocations, and normalized
SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
