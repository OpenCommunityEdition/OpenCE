# `rasterizer_xbox_profile.obj` remaining strict boundary

## Disposition

No remaining function meets the evidence threshold for a single strict
production candidate. This wave compiled zero candidate bodies and changed no
C source, symbol configuration, compiler option, comparator, exception,
admission record, or parked record. The object remains at 7/16 strict
functions, 352/2,511 meaningful bytes, and 400/2,640 padded bytes.

The boundary is not lack of semantic understanding. January and the Stian /
Pastudan cachebeta reconstructions establish almost all behavior. The blockers
are source-level expressibility under the campaign rules: a private ESI/EDI
call contract that cannot be declared for one isolated ordinary-C function,
donor bodies that explicitly use prohibited `volatile` or codegen tuning,
unrecovered typed owners at the tail of the profile BSS state, and one
cross-translation-unit ABI contradiction. A speculative one-shot compile
would therefore test an unauthenticated spelling rather than an original
source hypothesis.

## Scope and compiler contract

- Integration base: `3070c4434e5daef732115edca92c0e23dfa5ce2e`.
- Source translation unit:
  `source/rasterizer/xbox/rasterizer_xbox_profile.c`, Git blob
  `3de088d4f3ed55eb4aaac75534d3af5990404356`.
- January split object SHA-256:
  `f82d8d4735120015051332f2d243563d7ad32ba883a0a000b25bf9f3d418e2b1`.
- Compiler edge: XDK 3911 VC7 `CL.EXE`, `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`
  with the repository include graph recorded in `build.ninja`.
- Hardened authority: January padded COMDAT bytes, symbol storage, and ordered
  relocation destinations as measured by `tools.coff_compare`.

The three existing object ledgers were read before this classification. The
Stian and Pastudan `CLAUDE.md` / `AGENTS.md` rules, campaign README, audit
README, and object-ledger requirements were also applied. No archived working
tree carried a later committed profile body: the clean private-helper and
small-query branches both stop at the already integrated string-getter
baseline.

## Authenticated donor evidence

The clean Stian full-history repository is at
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`.

- `src/halo/rasterizer/rasterizer.c` blob
  `d88e85157b1b449868df580b436e1b9c56425ce2` reconstructs the warning
  helper, both D3D callback helpers, frame begin, both queries, and frame end.
- `src/halo/rasterizer/xbox/rasterizer_xbox_profile.c` blob
  `5c3b5352aa0d1de731a535fdfc486e234d549d1b` reconstructs profile begin
  and end.

The independent Pastudan full-history repository is at
`918af885935ec470a31256ecce9a977b12b01f80`; its Xbox profile donor blob
`22b2f5b2865316efd3403dd576b52070e49247e4` records the same begin/end
topology. These are functional cachebeta lifts, not byte-exact January source.
Their own retained commit subjects report these VC71 ceilings where recorded:

| PC donor | January counterpart | Donor result |
| --- | --- | ---: |
| `FUN_0016f500` | `_code_0015ed50` | 95.4% VC71 |
| `FUN_0016f730` | `_rasterizer_profile_frame_begin` | 91.1% VC71 |
| `FUN_0016f910` / `FUN_0016fa40` | profile begin/end pair | 95.5% VC71 |
| `FUN_0016fbd0` | `_rasterizer_profile_query` | 91.3% VC71 |
| `FUN_0016fcf0` | `_rasterizer_profile_query_pushbuffer` | 96.3% VC71 |
| `FUN_0016fdd0` | `_rasterizer_profile_frame_end` | 94.6% VC71 |

Those percentages are donor provenance only and grant no January credit.

## Complete remaining inventory

| Rank | Function | Meaningful / padded | Relocs | January normalized SHA-256 |
| ---: | --- | ---: | ---: | --- |
| 1 | `_code_0015ee60` | 136 / 144 | 10 | `abc3c4e52425069200126946f7eeafc9426b397f8e1f1926984417bcf2f4858a` |
| 2 | `_rasterizer_profile_frame_end` | 209 / 224 | 21 | `8d75c71e38a6a2ce53271ec9c03fafd5f2c7f43aace664cfce241b47ae158277` |
| 3 | `_code_0015ecd0` | 114 / 128 | 11 | `edf9b4c390ce6b95ccc9e62cca3149939119ecd18bc0d62622cd1d2693804eb1` |
| 4 | `_rasterizer_profile_begin` | 295 / 304 | 29 | `060cd6b4e0e7d234b4275fbfabc3e1efbf91394c236eebe619b791826d0f9583` |
| 5 | `_rasterizer_profile_query_pushbuffer` | 215 / 224 | 17 | `88a1eaee0462c703e039b38c09901fc4ebe6f77ab0b7205791dc9250acd62bee` |
| 6 | `_rasterizer_profile_query` | 286 / 288 | 22 | `35d38e566f8e229093092b0fdeab2cbd7fe8d510b64b7cdfffdb5a0f45dfa74e` |
| 7 | `_rasterizer_profile_frame_begin` | 330 / 336 | 35 | `ceb5881523138ae04fcd2ab92bfbd678cbfc8cee0009f38e7aad66147a987fe1` |
| 8 | `_code_0015ed50` | 264 / 272 | 22 | `49342dca6424bd0f00f61bed9d4e52e585513abb98ed12758f9c1e863f23cb0e` |
| 9 | `_rasterizer_profile_end` | 310 / 320 | 31 | `2e15770b556be5059d2a7ca4bc368c7dc06ea6ea72228a9aed5c162a292d4d59` |

Rank is expected leverage if new evidence appears, not permission to compile.

## Ranked boundaries

### 1. `_code_0015ee60`: best semantic leaf, typed-owner boundary

January and Stian agree on a plain `void(unsigned long)` callback. The low bit
selects start/stop and the signed short at `marker >> 1` selects one of 16
slots. It calls `QueryPerformanceCounter`, stores the start or stop value, and
on stop writes `stop - start`, plus the last slot index. January's ten
relocations fully authenticate the three 16-element `__int64` arrays and the
word store at `_rasterizer_profile_start_times + 252`.

This is the best future leaf, but not an admissible candidate today:

- the two callback arrays and the elapsed owner are currently declared
  `volatile`; using those types in a new body crosses the campaign's absolute
  no-volatile boundary;
- the last-slot word is at `+252`, beyond the current 29-element start-time
  array's typed extent of 232 bytes; accessing it by arithmetic, cast, or an
  overlapping incompatible view would violate named-field and no-punning
  rules;
- no source/PDB evidence authenticates a complete owner type or a standalone
  name for that word, and changing the existing BSS definition into a larger
  aggregate would be a separate ownership recovery, not a leaf candidate.

The exact `_collision_log_start_time` and `_collision_log_end_time` are useful
same-compiler controls for ordinary `LARGE_INTEGER` QPC calls and `__int64`
subtraction. Their hashes are respectively
`63bf78a6f07971df04769bae2ba18a6b90615e87a71be92f83beb002afa345bf`
and `8ddd5180ba30cb5e82c2c3420ead82757ac792dd3550e4cf75441f8113acec7d`.
They do not resolve this profile owner's qualifier or tail layout.

Reopen only with an authenticated full profile-BSS type/name boundary that
permits non-prohibited named access, or original source proving a policy-safe
owner declaration.

### 2. `_rasterizer_profile_frame_end`: ABI and donor-exactness boundary

Stian fixes the complete behavior, and its PC function has the same 224-byte
address span as January. January negates a frame `__int64`, saturates only the
positive overflow case to `LONG_MAX`, stores a sign-extended result, registers
`_code_0015ee60` with an odd frame marker, computes a floating ratio from a
callback elapsed value and the QPC frequency, then calls
`_profile_rasterizer_stats`.

Two independent blockers remain:

- the donor is documented at 94.6% VC71, so it does not freeze an exact source
  spelling or lifetime shape;
- January pushes 12 bytes to `_profile_rasterizer_stats` -- a `real` and an
  `__int64` -- while the current exact callee is declared and defined as
  `(long, long)`. Its exact hash
  `d2a88f26be45877e903a87316d6f952be36c8f04e11a7facf5062f7e448856e4`
  confirms it reads only `[EBP+8]` and `[EBP+0xC]`, but that does not make an
  incompatible caller declaration defined C. Correcting the ABI requires a
  separately reviewed cross-TU signature recovery and callee regression gate.

This body also needs named fields at start-time owner addends `+232`, `+236`,
`+240`, `+244`, `+248`, and `+252`, so the rank-1 ownership boundary applies.
Reopen only after the BSS owner and `profile_rasterizer_stats` prototype are
authenticated together, and an exact same-compiler source fixed point resolves
the donor's 94.6% residual.

### 3-6. Warning helper and its callers: private call-contract boundary

January `_code_0015ecd0` reads the message from ESI and the profile index from
DI, while only the boolean condition is present at `[EBP+8]`. Its callers load
ESI/EDI and push exactly one argument. This is a compiler-internal same-TU
contract, not cdecl, stdcall, or fastcall. An isolated external C prototype
would necessarily push the message and profile too; a one-argument prototype
would leave their reads without C parameters. Register annotations, inline
assembly, naked functions, and synthetic globals are prohibited.

The same boundary directly blocks `_rasterizer_profile_begin`,
`_rasterizer_profile_query`, and `_rasterizer_profile_query_pushbuffer`.
Profile begin and both queries relocate to `_code_0015ecd0` after loading the
implicit ESI/EDI operands. The queries additionally consume the currently
volatile elapsed-time owner. No isolated function in this set can reproduce
January while the private helper remains unwritten.

Reopen as a separately authorized natural same-TU cluster only when original
source or an exact VC7 control proves that defining the helper and every live
caller together regenerates the private contract without `register`, ABI
attributes, or other steering. Do not probe individual external declarations.

### 7-9. Frame begin, timestamp callback, and profile end: prohibited controls

- Stian's `_rasterizer_profile_frame_begin` counterpart explicitly uses
  volatile sticky-error reads/clear and documents a hoisted frame-slot read plus
  store ordering chosen from disassembly. Its 91.1% VC71 ceiling proves that
  even those controls do not freeze January source.
- `_code_0015ed50` uses volatile warning-word load/modify/store sequences and
  deliberate dead re-reads to approach its 95.4% donor result. Those are
  precisely the volatile/byte-forcing controls excluded by this campaign.
- The profile-end donor adds a volatile stack spill solely to reproduce a
  register/lifetime choice, reaches only 95.5% with its sibling, and still
  calls `_code_0015ecd0` through the private ESI/EDI contract.

Reopen only with original source or a clean exact donor that removes the
documented controls and proves the natural C89 topology. Do not transfer the
donor's volatile qualifiers, dead loads, hoists, spill variables, or permuter
shape.

## Do not repeat

- Do not compile any of the nine Stian/Pastudan bodies verbatim: they contain
  raw addresses, offsets, casts, incompatible declarations, or explicit
  codegen controls and are documented non-exact where measured.
- Do not declare `_code_0015ecd0` as external cdecl/fastcall/stdcall, invent an
  ESI/EDI ABI, or reconstruct it with register variables or assembly.
- Do not index beyond `rasterizer_profile_start_times[29]`, create an
  overlapping typed alias, split an `__int64` with pointer casts, or inject an
  unauthenticated interior BSS symbol merely to enable codegen.
- Do not remove semantically meaningful volatility from callback-shared state
  to satisfy the source policy, and do not add new volatile access as a
  compiler steering device.
- Do not repair the `profile_rasterizer_stats` caller by giving this TU a type
  that conflicts with the exact callee. Recover and gate the ABI at both ends.
- Do not use donor fuzzy percentages, equal address spans, or semantic
  equivalence as exact-match admission.

## Validation baseline

Because no production source or configuration changed, the previously gated
state remains authoritative:

- seven exact, zero residual candidates, and nine unwritten functions;
- all seven exact profile siblings preserved;
- strict padded board: 277/619 complete objects, 4,753/8,246 functions, and
  659,703/1,922,669 padded bytes;
- semantic audit: 470 units, 4,887 functions evaluated, 4,771 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,781 accepted exact, zero errors;
- campaign progress: 383/833 objects, 4,742/11,060 functions,
  602,983/2,198,102 meaningful code bytes, and 1,856,018/4,176,062 data bytes;
- admission zero candidates / contradictions / revocations; parked validation
  12 active / zero stale / zero invalid; tooling tests 205/205;
- protected `_unit_preprocess_node_orientations`: 1,920 padded bytes, 87
  relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.

This boundary ledger is additive evidence only and claims no function, data,
or whole-object credit.
