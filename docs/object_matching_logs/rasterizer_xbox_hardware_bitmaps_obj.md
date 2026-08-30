# `rasterizer_xbox_hardware_bitmaps.obj` upload-cluster closeout

## Result

The January Xbox hardware-bitmap translation unit is now at an evidence-backed
ordinary-C fixed point. The production reconstruction advances the object from
7/19 to 18/19 strict-exact functions, from 144/2,448 to 2,048/2,448 padded code
bytes, and recovers the complete 84-byte format/face const owner exactly. The
remaining 400-byte `rasterizer_bitmap_new` is parked as a VC7
branch-threading/block-layout tie; it is not credited as exact.

The honest object disposition remains `NonMatching`. No semantic exception,
compiler flag, assembly, volatile/register forcing, pragma, barrier, undefined
behavior, raw-address access, or byte patch is used.

## Authority and provenance

The research blueprint is
`docs/object_matching_logs/rasterizer_xbox_hardware_bitmaps_obj_upload_helpers_blueprint_20260830.md`
at commit `7c982f151`. It preserves the pre-edit baseline, all January section
measurements, exact strings and relocation schedules, XDK line references,
HCEA cross-build corroboration, and the bounded acceptance policy. No pre-edit
regression manifest survived; the blueprint's immutable measurements and the
final 19-function gate are therefore the baseline/regression evidence. This
ledger does not claim a missing pre-edit snapshot.

The immutable January split artifact recorded by the blueprint has raw SHA-256
`ac2ed2f8928478c833b5f3431c3668fcde7da1f66080fc982f861532240d9968`.
Strict authority is the hardened COFF comparator, not raw object identity.
Production compilation uses the repository's pinned XDK 3911 CL
13.00.9254.1 edge and flags.

The positive reconstruction evidence is:

- January code, data, relocation, ABI, string, and private-ownership shape;
- the stock XDK 3911 `D3DINLINE` create/lock/unlock/release bodies from
  `xbox/include/D3D8.h`;
- the authenticated HCEA checkout at `research/halocea`, commit
  `c89106c4964f2df3a98ef7b4035d6750372797cd`, for bitmap dispatch, mipmap
  dimensions, compressed/linear upload behavior, volume slice stepping, cube
  face order, and the independently declared `short face_mapping_table[6]`;
- the repository's existing exact swizzle and bitmap helper interfaces; and
- more than 50 measured ordinary-C control-flow probes for the final
  `rasterizer_bitmap_new` edge.

The user-provided historical discussion that Bungie's fast `ftol` helper was
probably handwritten is relevant to that specific math helper only. It does
not provide source or permission for assembly in this rasterizer object.

## Recovered code and ownership

All 13 stock XDK inline wrappers are strict exact and are recorded as private
symbols in `config/symbols.json`:

- the three device texture constructors;
- the base-texture release wrapper;
- the texture, volume-texture, and cube-texture lock/unlock wrapper cluster.

The three private upload helpers are strict exact:

| Helper | Padded bytes | Relocations | Result |
| --- | ---: | ---: | --- |
| `_rasterizer_bitmap_2d_changed` | 448 | 25 | strict exact |
| `_rasterizer_bitmap_3d_changed` | 528 | 26 | strict exact |
| `_rasterizer_bitmap_cube_map_changed` | 528 | 26 | strict exact |

`_rasterizer_bitmap_delete` remains exact at 48 bytes, and the recovered public
dispatcher `_rasterizer_bitmap_changed` is exact at 176 bytes. The dispatcher
and helpers preserve the target's private 3D calling convention, exact guard
order, loop-carried success byte, error sites, natural XDK unlock calls, and
format-specific swizzle/copy behavior.

The symbol map marks all 13 XDK wrappers, all three upload helpers, and the
format table with `"static": true`. This makes csplit emit private storage
class 3, matching the compiler-emitted private owners in the base object;
leaving those csplit owners external at storage class 2 would make the
whole-TU ownership claim false even though the individual bytes compare exact.

## Exact shared data

The source declares one contiguous private aggregate:

- 18 four-byte D3D formats (72 bytes); followed by
- six two-byte cube-face mappings (12 bytes).

The resulting owner `_rasterizer_bitmap_format_table` is strict exact at 84
bytes, zero relocations, normalized SHA-256
`4303e8a36fe876148b5717d0229e95c72364de0d74bda7fb48cd44be57852f0b`.
The cube helper's reference lands at table +72. The face mapping remains
`short[6]`; spelling it as the four-byte XDK enum would corrupt the target
layout.

## Accepted ordinary-C controls

The closest source is also the readable source:

- stock typed XDK calls, including natural no-code unlock calls;
- proven narrow `short` dimensions and cube-face mapping;
- one aggregate format/face data owner;
- explicit mipmap loop latches and shared failure fallthroughs matching the
  January CFG;
- the 3D pixel-size query and division by the cached depth inside the slice
  loop, where the target emits them; and
- a normal public bitmap-type switch which lets VC7 reproduce the target
  private helper convention.

The exact 2D, 3D, and cube control-flow recipes are preserved in untracked
lane scratch during development but are not production inputs. Only the C
source, reviewed symbol ownership, parked evidence, and this ledger are
committed.

## Final `rasterizer_bitmap_new` fixed point

The residual has equal 400-byte padded sections and all 26 relocation
identities and addresses. Target normalized SHA-256 is
`041cef84e2a13eeae0fccfaef6e8e897a5c5926bf850bc6c10bc245f3df8a82d`;
base normalized SHA-256 is
`719469460357f06cde7f452e16e02ba79ac639e85e0fad244f8e01cb7bd5efcd`.
Objdiff reports exactly 99.96377%.

The target and base are instruction-identical through +0x159. The sole
normalized difference is the displacement byte at +0x15A of the `jne` at
+0x159: January selects the first `hardware_format = NULL; return TRUE`
epilogue at +0x171, while current VC7 selects a byte-identical second epilogue
at +0x17E. The emitted behavior is identical.

More than 50 legal-C probes covered shared versus split returns, direct and
inverted predicates, explicit success materialization, goto/fallthrough
topologies, duplicate and single cleanup blocks, declaration/lifetime changes,
and combinations of those families. Every form either retained the one-byte
branch destination or regressed earlier exact code, sizes, or relocations.
The plain semantic form is retained and the function is parked under the
allowed `instruction-scheduling` class. Parking records the compiler fixed
point; it does not convert the function or object to exact.

## Do not repeat and reopen criteria

Do not repeat generic return, predicate, goto, cleanup-block, declaration, or
lifetime sweeps for `rasterizer_bitmap_new`. Do not split or reorder the
84-byte const owner, widen the cube mapping, hand-write the XDK wrappers, omit
the unlock calls, move the 3D size queries out of the slice loop, or force code
generation with assembly or compiler tricks.

Reopen the parked edge only with original January source/local/PDB provenance,
an authenticated cross-build donor with the same block layout, or a
demonstrated ordinary-C VC7 control which changes only this branch-threading
decision while preserving the exact prefix, size, relocation schedule, exact
siblings, and data ownership.

## Verification

Final production gates on 2026-08-30:

- focused 19-function gate: 18 exact, one residual, zero unwritten;
- exact code census: 2,048/2,448 padded and 1,875/2,263 meaningful bytes;
- report data: 1,584/1,584 matched; the 84-byte owner is strict exact;
- complete `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass;
- semantic report: 470 units, 4,974 functions evaluated, 4,857 semantic exact,
  131 hidden exact, zero ordinary rejected, and zero unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked audit: 13 active, zero stale, and zero invalid;
- tooling suite: 212 passed;
- protected Units sentinel `_unit_preprocess_node_orientations`: strict exact
  at 1,920 padded bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- prohibited-source scan: no matches; and
- `git diff --check`: pass.

After integration, a clean post-closeout whole-TU regression snapshot/check is
required to prove reproducibility without overstating the missing pre-edit
manifest.
