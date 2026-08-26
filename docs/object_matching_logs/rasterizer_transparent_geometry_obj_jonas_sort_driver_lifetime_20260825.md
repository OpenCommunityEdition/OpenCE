# `rasterizer_transparent_geometry.obj` sort-driver lifetime one-shot

## Result

This Jonas/Codex ledger records one bounded, failed-closed source-lifetime
experiment for
`source/rasterizer/rasterizer_transparent_geometry.obj::_code_00174120` from
clean cumulative commit
`b6d0ccd083ae7118ac57e57d8992eac89ef088c5`. The candidate made causal
progress but did not satisfy strict semantic-COFF equality. It was removed
immediately, the immutable baseline object was restored, and no implementation
or matching credit survives.

The unit remains 15/19 accepted functions. The residual remains 173 meaningful
/ 176 padded bytes with 14 relocations.

## Provenance and prior documentation

The January split object is the byte and relocation authority. Its raw SHA-256
is `d1bcf0d85a17c27f0cb9797931187fad486c2ac7b8ce7bcb43e706422c160722`.

Before freezing the shot, the complete object-specific Claude research and the
relevant compiler-allocation guidance were read from
`claude/near-complete-objects-20260816` at
`74a085e90ae66511c22820a8f5256b3273342542`:

- `research/transparent_geometry_recovery.md`, blob
  `52d38e05638dac26c4eb658a7849a4f5a68869b5`;
- `research/transparent_geometry_sort_comparator.md`, blob
  `1e5d6c93ed106ff9a7b9d694424884269d19e074`;
- `research/register_tie_reachability.md`, blob
  `1798df9e9e2fc5ade819a1d7b49d8fd84bc283e3`;
- `research/register_tie_cumulative_findings.md`, blob
  `0621651f58224429a724262580c9a7d25c2d42f4`.

The first two records establish the sort-driver topology, the real in-TU
private assert-helper relationship, the typed group and sorted-index storage,
and the comparator call. The register documents establish that definition and
lifetime order are legitimate source levers for integer allocation, while a
synthetic probe cannot authenticate a whole-function schedule.

The independent HCEA reconstruction at commit
`0fd88176fe19df400576d2cbb447d3363080cb52` supplies the readable identity-fill,
`qsort`, and rank-stamping topology in `src/rasterizer_sort_external.c`, blob
`8b7fab90eb931cd6e35ed2874219fd28d7de75ab`, payload SHA-256
`c186863a177fa5e40eabbd9860d6f9d4af54286942b0359f6e5ff81d7576c056`.
HCEA is a later Xbox 360/PPC reconstruction and is not a January-PC byte
oracle; it supports only the high-level source topology.

The current production object was reconstructed after Claude's target-only
frontier measurement. Reachable history and workspace census contained no
recorded compile of this exact direct-expression, block-local-pointer, and
pre-test-index lifetime combination.

## Frozen baseline

The clean source identity was Git blob
`f278c3493a9aeb17d83e3d5cdd5aa382183459f4`, with CRLF working-tree SHA-256
`874d31a32c82f5ac75330c668444e44cd4a12fd5fe12f84305a7e4badb6bfd5a`.
The isolated worktree was configured with the repository's XDK 3911 graph and
Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1. `CL.Exe` SHA-256 is
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
the production flags remained `/O2 /Oy- /DDEBUG /Dxbox` and the generated
include closure.

A clean pre-edit baseline compile established the local Ninja command record.
The frozen baseline object and immutable copy both have raw SHA-256
`906223ee9cf34ea1186f292d3ca1d4c92a031ea54918ca3a419bc731ce303581`.
The regression manifest
`build/audit/rasterizer_transparent_sort_driver_baseline_20260825.json` is
400,979 bytes with SHA-256
`7997a12c7fd4139c7f769612dcfc04a92c15ac60b879a636d99d27ce17c354aa`.
Its self-check passed with no failures or warnings and froze these 15 inherited
accepted functions:

- `_code_00173f70` and `_code_00173fa0`;
- `_rasterizer_transparent_geometry_begin`, `_dispose`, and `_end`;
- `_rasterizer_transparent_geometry_get_group_from_presorted_index`;
- `_rasterizer_transparent_geometry_get_group_pending_status`;
- `_rasterizer_transparent_geometry_get_group_presorted_index`;
- `_rasterizer_transparent_geometry_get_groups` and `_get_groups2`;
- `_rasterizer_transparent_geometry_get_primary_vertex_type`;
- `_rasterizer_transparent_geometry_new_group` and `_new_group2`;
- `_rasterizer_transparent_geometry_set_group_pending_status` and `_stop`.

The residual's target and baseline fingerprints were:

| Side | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `173 / 176` | 14 | `4f78b8476535a901325781b67d7ed04665e59478fb419ddd0c607c95220655d5` |
| clean baseline | `173 / 176` | 14 | `30cd227ef42506ffe5a4050491eff57faef6a200cbffc05301077bca69f40a50` |

Their normalized bytes differed at 69 offsets, first at `+0x05`. The target
saved only ESI/EDI at entry, preserved the first count in EAX, zeroed the second
pass index before testing the count, and shrink-wrapped EBX around the
nonempty second pass. The baseline saved EBX at entry, clobbered the first EAX
count with the named group pointer, initialized the second index after the
test, and loaded the sorted-index pointer before the group pointer.

## Frozen candidate and one production compile

The one candidate made three ordinary-C lifetime changes in the existing body:

1. removed the function-scope `group` pointer;
2. passed `transparent_geometry_groups + group_index` directly to the private
   assert helper in the first pass;
3. initialized `group_index` before the post-sort positive-count test and
   declared `group` before `group_sorted_indices` inside that positive block.

The candidate source was Git blob
`53da9b0b8fd6613f0d41c34c4ac0b6a8d4186e6c`, with working-tree SHA-256
`e317b4b434495c7fc6515bdd5817b49f9dfd34a69d2bac1c34e1f336f4311f15`.
It used only named typed members and ordinary control flow. It introduced no
assembly, `volatile`, `register`, pragma, annotation, raw address, byte offset,
pointer/integer reconstruction, object-representation cast, type pun, undefined
behavior, synthetic anchor, or compiler option change. The file's inherited
packing pragmas were unchanged and outside the patch.

One XDK `/Zs` invocation with the exact generated include closure passed and
left both baseline object hashes unchanged. The following dry run exposed
exactly one code-producing edge:

```text
[1/1] CL build\base\source\rasterizer\rasterizer_transparent_geometry.obj
```

Exactly that edge received the wave's sole production candidate compile. The
immutable first-shot object has raw SHA-256
`5f8e8cf790033dc6af3e78b64c253b59d676c2e47ee741537fa5f5a43612d51a`.

## Measured rejection

The candidate retained the target's `173 / 176` size and 14-relocation count,
but its normalized SHA-256 was
`c7a86bc76b0c47bf55982cca4a409ba259335ac1840eecf088b53392dc425446`,
not the target hash. The normalized distance improved from 69 to 58 differing
bytes and the first divergence moved from `+0x05` to `+0x11`, but strict
equality is binary and no fuzzy credit is claimed.

The change did explain part of the target:

- the entry prologue now saved only ESI/EDI;
- the second-pass index was zeroed before the count test;
- EBX was shrink-wrapped around the nonempty second pass;
- the loop body from `+0x90` through the epilogue aligned with January.

It did not reproduce the remaining schedule. In the first pass January formed
the transient group in ECX from a base held in EDX, while the candidate formed
it in EAX from a base held in ECX. Around `qsort`, January pushed the comparator
and width before loading the sorted-index pointer; the candidate loaded that
pointer first. The candidate's post-sort prefix was therefore one byte later
until alignment at `+0x90`.

Relocation addresses make the residual explicit:

| Side | Relocation addresses |
| --- | --- |
| target | `1,18,40,45,50,57,62,71,88,96,102,108,123,130` |
| baseline | `1,18,40,45,50,57,66,75,89,94,103,109,122,128` |
| candidate | `1,18,40,45,50,57,66,75,89,94,103,109,124,131` |

The full regression check reported every inherited accepted function as
`still_exact` and no newly exact function. The only non-code change was the
compiler-owned anonymous `.debug$F` section caused by the source-local change;
no runtime-data or symbol-ownership gain is claimed. Because the intended
function remained nonexact, no debug adjudication, full Halo/libcmt validation,
status change, or replay chain was permitted.

## Restoration and reopen boundary

The candidate was inverse-reverted without a source-shape retry. `apply_patch`
changed the working-tree line-ending representation, so the clean cumulative
copy was used solely to restore the CRLF bytes. The live source is again Git
blob `f278c3493a9aeb17d83e3d5cdd5aa382183459f4` and raw SHA-256
`874d31a32c82f5ac75330c668444e44cd4a12fd5fe12f84305a7e4badb6bfd5a`.
The immutable baseline object was copied back; live and immutable raw hashes
again equal
`906223ee9cf34ea1186f292d3ca1d4c92a031ea54918ca3a419bc731ce303581`.
A one-target Ninja dry run reports no work, the manifest check again passes
with all 15 inherited functions `still_exact`, and the tracked tree contains
only this ledger.

Do not retry the compiled direct-expression + block-local-pointer + pre-test
index family, or split it into spelling variants, without genuinely new
January source or another compiler-authentic artifact that explains both the
first-pass ECX/EDX assignment and the pre-`qsort` relocation order. HCEA's
semantic loop topology and allocation reachability alone are now exhausted for
this source family.
