# `geometry.obj::_convex_polygon3d_verify` one-shot boundary (2026-08-29)

## Result

This strict leaf lane starts from canonical commit
`ca0846407cf8c216e4682a96573eacbf5998533c`. Its sole production candidate
was `source/math/geometry.obj:_convex_polygon3d_verify`, selected because two
independent public-source trees agree on the natural algorithm and January's
packet is link-closed. The ordinary typed C reconstruction was compiled once
under the unchanged production edge. It did not reproduce the complete
January packet, so the source and owner-header declaration were removed
immediately without a retry.

No production source is retained. `geometry.obj` remains 7/30 strict exact
functions and remains `NonMatching`; this ledger grants no byte, function,
data, or object credit.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January `_convex_polygon3d_verify` | `387 / 400` | 1 | `81f3aa0a38d1455224d92093fd8e930e2074b2d3fac4725c651454bc4c8c3e4b` |
| First and only candidate | `371 / 384` | 1 | `7fc5baf19d72b5ecc59710f5268424e7b66df8d149d37cbeb47ec69f57e90dc8` |

The relocation identity is concordant but its location is not: January has a
type-`DIR32` reference at `+342` to defined non-code literal
`__real@b58637bd + 0`, while the candidate places the same reference at
`+324`. Complete padded bytes and ordered relocations therefore compare
`all_equal: false`.

## Selection, documentation, and provenance audit

The repository contains no `CLAUDE.md` or `AGENTS.md` at this base. The
complete current `docs/matching_methodology.md`, both prior `geometry.obj`
ledgers, and the applicable earlier leaf-boundary ledger were read before
selection:

- `docs/object_matching_logs/geometry_obj_codex_checkpoint.md`;
- `docs/object_matching_logs/geometry_obj_jonas_register_cleanup_fixed_point_20260829.md`;
- `docs/object_matching_logs/error_geometry_obj_jonas_dispose_first_shot_20260826.md`.

The public Stian tree's complete `AGENTS.md` and `CLAUDE.md` were also read.
They are source-tree context, not an override of this repository's strict
admission policy. Their only substantive difference is the local skill path.

The function is unattempted in reachable source history: `git log --all
-Sconvex_polygon3d_verify -- source/math/geometry.c` returns only initial
inventory commit `e830efbd7c950c5b23efad2cbcdb52967b39cb0d`. No existing object ledger
records a candidate compile for this owner.

Two independent public sources authenticate the same semantics and broad
topology:

- HCEA commit `c168af2e747d3095d9a29418ae401f3a39544863`, file
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halocea-reference/src/convex_polygon3d_verify.c`,
  SHA-256
  `fd517da0d37e95e41a0c17ffaa42d8c65954f76522b00a009942ea6f68a1caab`;
- Stian commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, file
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/stian-halo-current-20260827/src/halo/math/geometry.c`,
  SHA-256
  `7841253e11d8c77bd5d7bd7e1a63853e2729dfff3d25a0fc3c8d0f3d9e8264e3`.

Both compute a reference cross product from the first three vertices, walk
the polygon with a signed short index and wraparound previous/next points,
reject non-finite current points, and reject a corner dot product below
`-0.000001f`. January disassembly independently confirms those semantics: a
short `DI` loop, explicit finite masks, corner cross/dot arithmetic, and the
single negative-epsilon literal relocation. The target packet has no call or
other external dependency, so the candidate was naturally link-closed.

An earlier possible S3TC leaf was rejected before any edit or compile because
its public body calls absent same-TU `_DecodeBlockRGB`, which itself calls an
absent private helper. The exclusions assigned to this lane, including Units,
Vehicles, Matrix Math, AI Debug, Motion Sensor, Path Smoothing, LRA Cache,
Collision Usage, rasterizer profile remainder, HS macro evaluate, Winsock
error-string, AI Script, `network_game_manager::add_player`, and
`flags::_flag_new`, were not edited or compiled.

## Frozen production edge and first artifact

The compiler is Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 from
XDK 3911, 81,920 bytes, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
The unchanged geometry edge uses `/O2 /Oy- /DDEBUG /Dxbox` and the normal
repository include graph. There was no alternate compiler, flag, object path,
syntax-only precursor, or speculative variant.

Before the source edit, the January split object was 32,572 bytes with raw
SHA-256
`1afce0545c1e6d766152acf21abd191958962382ec390cfa831e056bae63e451`.
The seven inherited strict packets compared equal before the edit. They total
528 padded bytes and 20 ordered relocations:

| Inherited exact owner | Padded / relocs | Normalized SHA-256 |
| --- | ---: | --- |
| `_convex_hull2d_area` | `96 / 2` | `76adbb29b34b3a1cc71d71dc76e776c4d57177697176fb9f25e91f6d863b03cd` |
| `_convex_hull3d_verify` | `16 / 0` | `1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707` |
| `_convex_polygon2d_verify` | `96 / 0` | `a41f70bc90c679b8067c07e7b359b285449ce3bf400d269a29109b94f6f24dad` |
| `_geosphere_dispose` | `176 / 18` | `6b0637bddb65eb86f4cfe62570afd1ffdc97f2b32047e0357830c24f6f5b2a21` |
| `_plane2d_distance_to_point` | `32 / 0` | `f36d80003aafc8fd5a9de075a2e300ee11d42e7b5604688a1211112e50b1af53` |
| `_vector_intersect_plane2d` | `48 / 0` | `36d5a7829f21625455b9e603fe8bd61328e4a5e0a77458d89f699625244fed2d` |
| `_vector_intersect_plane3d` | `64 / 0` | `7a44f94ceb3e1e8ef5297ab442fa97715c86c915883d3f496f9b6beeda23eced` |

The one natural reconstruction used an owner-header prototype,
`real_point3d const *`, named components, `valid_real_point3d`, a signed short
index, direct wraparound aliases, and the public reference-cross and
corner-cross/dot expression topology. The candidate source payloads before
revert had SHA-256
`cec02b56fdab07dcb42d9540a9472610ba1cc089171c1a67736f30512b351093`
for `geometry.c` and
`ff92421afc1b975635ee1d44bbe58e0d91e90360186b6591540b982af307f2d6`
for `geometry.h`.

The sole production invocation reported exactly:

```text
[1/1] CL build\base\source\math\geometry.obj
```

At measurement time the ignored first artifact was
`build/base/source/math/geometry.obj`, 5,713 bytes, raw SHA-256
`d369a809ba0a5a89cb84a939eab524e72d4f8347d503adaa06c53728e3d7b1a4`.
It was not an admitted build product. The initial baseline object had been
borrowed through a hardlink rather than copied; after the complete comparison
and disassembly evidence above had been extracted, a later source-identical
base rebuild through the other link replaced this ignored file. The current
path is therefore not the candidate artifact, and this ledger does not claim
that the raw object remains available. The normalized packet hashes, sizes,
ordered relocation records, and first-divergence evidence were all captured
before that replacement.

Direct hardened comparison reports all seven inherited packets still exact
in this candidate object. Only `_convex_polygon3d_verify` is unequal.

## Structural mismatch and fixed boundary

The mismatch is structural from the prologue, not padding-only. January uses
`sub esp, 0x30`; the candidate uses `sub esp, 0x1c`. January begins reference
setup with `points[0] - points[1]`, retains a different x87 stack schedule,
and stores finite-check temporaries at different frame slots. The candidate
reorders that setup and reaches the epsilon reference 18 bytes earlier. Its
last non-padding return ends at `+371`, versus January at `+387`, accounting
for the 16-byte padded-size deficit.

Per the frozen one-shot rule, no expression reordering, lifetime experiment,
local-layout adjustment, retry, alternate compiler setting, or byte-oriented
steering was attempted. Reopen this owner only with genuinely new
authenticated source topology or compiler provenance that predicts the
observed `0x30` frame and x87 transition. Do not retry the HCEA/Stian natural
transcription represented by this artifact.

The candidate body and prototype were removed with no second compile. Final
Git blobs exactly equal the integration base:

- `source/math/geometry.c`:
  `6640c797bfbd2537630d442f78b55526b748de35`;
- `source/math/geometry.h`:
  `4026bb46f1583e99670c95367b84912df8b4143e`.

The retained repository change is this additive ledger only. It uses no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, force-inline
control, barrier, raw address or byte offset, pointer/integer reconstruction,
cast or union pun, inactive-union access, undefined behavior, synthetic
caller or anchor, byte patch, comparator exception, or semantic exception.

## Validation

- Direct `tools.coff_compare` on the first artifact reported the new owner
  unequal exactly as tabulated above and all seven inherited geometry packets
  equal. No exact packet was lost. As recorded above, the shared ignored
  hardlink was replaced only after these results had been extracted.
- Final working-file Git hashes equal both base blobs. `git diff` has no
  production-source change, no deleted path, and no path under protected
  Units or any other excluded object.
- The complete tooling suite passes: 212/212 tests.
- The carried no-build semantic audit for the source-identical canonical
  graph scans 470 units and 4,917 functions, with 4,800 semantic exact, 4,810
  accepted exact, and zero unit errors or ordinary rejections.
- Object admission reports zero candidates, contradictions, or revocations.
- Park validation reports 12 active entries, zero stale, and zero invalid.
- `git diff --check`, retained-path review, and source-policy scan pass.

No second candidate was compiled and no full production graph was rebuilt in
this lane after the rejected shot. The source-identical base already carried
a clean full report; the bounded miss path requires source reversion,
evidence, and an additive ledger rather than a redundant candidate rebuild.

No completion, admission, parked-object, or semantic-exception record is
changed. No push, amend, rebase, history rewrite, or worktree removal is
performed.
