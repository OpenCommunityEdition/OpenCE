# xbox_texture_cache.obj Jonas public quintet — 2026-08-28

## Result

This lane started from pushed commit
`81eeeb81d179bebb1367c22bcf29a8eced511ebc` and froze exactly five ordinary
cdecl public owners. Four were strict first-shot hits and are retained. The
format mapper produced identical normalized bytes, padded size, relocation
count, and normalized hash, but failed hardened symbolic relocation equality;
it and its candidate-only table declaration/constants were immediately pruned
without tuning or another candidate compile.

| Retained owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_texture_cache_new` | 175 | 176 | 22 | `786bcb082a68d0e9f188e729f18c1c8f5a08a5ddacb14ae0b395e159810160a1` |
| `_texture_cache_bitmap_new` | 130 | 144 | 6 | `7e0d2eccc3f98c690dd44ae0a29d79655e7006c01b92809a452f8fa4bf3bc226` |
| `_texture_cache_return_memory` | 89 | 96 | 10 | `a3a6e18a048008540b739b458f386dc01b4faea66dcc968a9b88abbe4086fa05` |
| `_texture_cache_bitmap_delete` | 58 | 64 | 2 | `fcdfbcf7fcee32e7d7577a9534c4bacae43e737b6a5f86950adb032513d5128f` |
| **Retained gain** | **452** | **480** | **40** | |

The object advances from 5/25 to 9/25 strict-exact owners and from 176/3648
to 656/3648 padded code bytes. It remains `NonMatching`. The inherited
`_texture_cache_delete`, `_texture_cache_open`, `_texture_cache_idle`,
`_texture_cache_flush`, and `_texture_cache_close` owners were independently
rechecked after the required post-prune rebuild and remain strict exact.

## Frozen scope, provenance, and ownership

Before editing, the canonical methodology and campaign/tooling documentation,
all prior Xbox texture-cache ledgers, relevant headers, the current target
COFF, and the clean Claude/Jonas worktrees were read. The conservative census
found no live dirty overlap on this source. Units, Vehicles, fixed-point lanes,
private-ABI helpers, request/debug bodies, and all other objects remained out
of scope.

January target COFF/disassembly was authoritative for layout, widths, source
lines, control flow, and relocation schedule. External evidence was semantic
and topological only:

- HCEA pin `c168af2e747d3095d9a29418ae401f3a39544863`; donor blob/SHA pairs:
  format `07bb6c7...` / `28939f...`, bitmap delete `ddf1519...` /
  `8639ed...`, bitmap new `ba0ceaa...` / `04762e...`, cache new
  `db5a095...` / `e174fc...`, return memory `1081a96...` / `1b0f56...`.
- Stian full-history HEAD `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, Xbox
  file blob `fae53c8...`, SHA-256 `007911c6...`.
- Pastudan full-history HEAD `918af885935ec470a31256ecce9a977b12b01f80`,
  file blob `44bf315...`, SHA-256 `76eeb464...`; relevant history commits
  `3f44f9ea...`, `44c3e7af...`, `7f4ea52f...`, `1119848c...`, and
  `bf2f8a28...`.
- Claude's `docs/bugs/texture-cache-corruption.md` was used read-only for the
  known layout/table hazards; no raw addresses or foreign-build offsets were
  imported.

The reconstruction uses a TU-local, compile-time-checked 0x30 Xbox bitmap
layout and the verified bitmap-group pixel-data file-offset path. The cache
global prefix names only the proven `+0x1604` base pointer. `XPhysicalProtect`
has a local typed declaration; `xtl.h` is not included. LRU callbacks
`code_001ae840` and `code_001ae880` remain declarations only and are passed
under the existing typed callback ownership. No private callback body or
aggregate data owner was defined.

The retained source contains no assembly, volatile/register scheduling device,
pragma, intrinsic, force-inline annotation, raw address, cast-based layout,
union pun, undefined behavior, synthetic anchor, or byte-forcing expression.

## One-shot and failure record

The production-argument `/Zs` parse-only check succeeded and emitted no
object. A Ninja dry run exposed exactly one candidate CL edge, which ran once
for the complete frozen quintet. Immediate preserved-object adjudication was:

- four retained owners: strict `section_infos_equal` true;
- `_bitmap_format_to_d3d_format`: false, despite candidate and target both
  reporting 144 padded bytes, 10 relocations, and normalized SHA-256
  `5a451e60010f82a483f6b92dd6d65f95678601a91172cda77c1925807e08ff70`.

That is a semantic relocation-identity residual, so the body and its
candidate-only `_rdata_002a7bd0` support were pruned immediately. There was no
spelling experiment, tuning, or second candidate compile. The required normal
post-prune rebuild then emitted the final object and all four retained plus all
five inherited owners re-gated exact.

The first full-tree validation attempt reached ordinary compilation but the
two repository-pinned tool edges attempted network downloads and failed under
the sandbox. The existing local copies were not accepted because the fresh
Ninja log had no matching command record. The pinned csplit v0.0.2 and objdiff
v3.3.1 binaries were then fetched through the authorized build edge; the
resumed 530-action build completed. This tooling failure did not compile or
alter the already current texture-cache object.

## Reproducibility and validation

- Isolated post-prune object
  `work/xbox-texture-cache-public-quintet-20260828/build/base/source/cache/xbox_texture_cache.obj`
  SHA-256:
  `49f2d8389f1aa9f35166c6b0dd86afbe28501f5ed6c07c4246c41befaea38e6c`.
- Coordinator final rebuilt object SHA-256:
  `9107f6626460e1aec598e02832b231343ca99979a3cfbfcbb3ced1dbb3b5b8fd`.
- January split object SHA-256:
  `457d6eb9f4e79e816f6539c4bdbe2555c957267e75fefef156fa48076f03dc0f`.
- Final source blob: `6c292b1662a1f8bfbbba5ffe63e2a02bfff3821d`.
- Isolated pre-normalization source SHA-256:
  `5eab57cbb045ad894a49a97304c5daf9de9a8ebe92d733dad722045640dbf6c3`.
- Coordinator CRLF-normalized source SHA-256:
  `c1f15128770217ce4613ad74ba2f368e33938bebb5a131f07bc8879351799f1c`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- VC7 compiler SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Flags remained `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the repository
  include paths and XDK 3911.

Validation completed:

- full Halo/libcmt build and generated ordinary/semantic reports: pass;
  semantic report scanned 470 units / 4,709 functions, found 4,580 semantic
  exact, 125 hidden exact / 72,363 hidden bytes, 4,590 accepted exact, and
  zero unit errors;
- strict campaign board: 273/619 exact objects, 4,561/8,246 exact functions,
  and 614,039/1,922,669 padded code bytes;
- object-admission audit: zero candidates, contradictions, or revocations;
- parked validation: 13 active, zero stale, zero invalid;
- complete tooling suite: 205 tests passed;
- `git diff --check`, changed-path review, deleted-path review, and prohibited
  construct scan: pass.

Exactly two tracked paths are changed: the source and this ledger. Nothing is
committed or pushed from the isolated lane. The coordinator's pre-existing
untracked `scratch/` evidence state is explicitly excluded from integration,
staging, and commit.
