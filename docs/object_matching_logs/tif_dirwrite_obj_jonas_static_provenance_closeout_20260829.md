# `tif_dirwrite.obj` authenticated static-provenance closeout (2026-08-29)

## Scope, base, and rules

This closeout starts from clean canonical commit
`cbc900ec836dc75fc97880cd7d819e45c6660326` in isolated worktree and branch
`jonas/tif-dirwrite-recovery-20260829`. It changes only
`source/bitmaps/libtiff/tif_dirwrite.c`, the three object-description files
`config/config.json`, `config/symbols.json`, and
`config/semantic_matches.json`, plus this additive ledger. It does not touch
Units, Vehicles, Matrix Math, AI Debug, another source unit, a header, a
compiler option, or a tool. Nothing is pushed.

The pinned compiler is XDK 3911 CL 13.00.9254.1, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
with the established `/O2 /Oy- /DDEBUG /Dxbox` Halo flags. The January
csplit object before symbol authentication had raw SHA-256
`c694573170454c8e86f939ff7a420c31f6d19727c28260977d067dba383c06a9`.
The cumulative baseline object had raw SHA-256
`35a02af2f6471d692c9e1e2c722ebe19771164bc4814a788f18fe091d8be1879`.
The ignored frozen baseline manifest is
`build/audit/tif_dirwrite_baseline_20260829.json`, SHA-256
`fd9c2289bb8a2707136e7c095e4366d4f947b7589a6a521a88939e14b75ff071`.

The baseline was 3/14 strict-exact functions, 533/4,083 meaningful code
bytes, and 606/606 logical data bytes. The inherited exact functions were
`_TIFFSetupShortLong`, the address-named `TIFFLinkDirectory`, and
`_TIFFSetupShortPair`. All three and every inherited data owner are protected.

## Complete donor and prior-attempt census

Every applicable campaign, Claude, AGENTS, TIFF, object-admission, semantic,
relocation, and tooling document was read before choosing a candidate. All
nine current TIFF object ledgers were read in full. In particular,
`tif_dirwrite_obj_jonas_declaration_order_20260825.md` proves that reversing
the two automatic declarations in `TIFFWriteRationalArray` is an inert
one-shot miss: 143 meaningful / 144 padded bytes, eight relocations, target
SHA-256 `e73a1566...`, and unchanged baseline SHA-256 `fb123b0e...`. That
shape was not retried.

The complete reachable source history contains four Git blobs: the authentic
upstream RCS 1.16 source (`1e9c831b...`), the first symbol/data recovery
(`96c48fe0...`), the Halo runtime/debug recovery (`8bda2375...`), and the
current baseline (`361094f2...`). Relevant commits include `e830efbd`,
`445e...`, `75a...`, `810b...`, `917...`, `45f8...`, `1a66...`, and
`3d8...`; no hidden fifth source topology exists.

The registered-worktree census covered 1,511 worktrees. Source existed in
1,510 of them and reduced to nine physical file hashes representing the same
four reachable Git blobs; no dirty or untracked source body supplied a new
candidate. Of 1,089 available compiled base objects, all normalized artifacts
reduced to three families. None supplied a strict unintegrated result after
owner and relocation identity were enforced. The two prior audit objects were
only the declaration-order baseline and its inert first shot. No additional
artifact was found under durable `artifacts`, `outputs`, `research`, or the
three temporary split-object mirrors.

The authenticated public donor is punpckhdq commit `e830efbd...`, whose
`tif_dirwrite.c` carries libtiff RCS marker `1.16 92/03/18`; its working-file
SHA-256 begins `5e54...`. Its historical private names, file-local function
ownership, and file-local `static long dataoff` are authoritative. Old
authentic-source objects reproduce ten January normalized bodies once these
private owners are resolved, which isolated ownership rather than statement
semantics as the missing fact. Pastudan commit `5449fca...` and head
`918af885...` are raw-address/ABI reimplementations and are excluded. HCEA,
Stian, Baboon, demon, nimbus, halopc-restored, and the remaining supplied Halo
trees/history contain no distinct `tif_dirwrite.c` donor. The bnunu history
contains only the same four blobs and ledgers.

## Single frozen production candidate

One coherent, source-version-backed candidate restores the authentic private
ownership model without changing the function algorithms:

- remove the address-name preprocessor aliases and use the readable upstream
  `TIFFWrite*` and `TIFFLinkDirectory` names;
- restore `static long dataoff;` instead of an externally visible anonymous
  BSS surrogate;
- authenticate the thirteen private functions and `_dataoff` as `static` in
  `config/symbols.json`, preserving public `TIFFWriteDirectory`;
- state the already-authentic signatures as ordinary explicit typed C89,
  one parameter per line, with `void` for the no-value
  `TIFFSetupShortLong` and an explicit `return;`; and
- leave every expression, literal, branch, loop, call, and enabled feature
  definition unchanged.

The frozen source is Git blob
`97514d726f63b9c78fbaa7ddc45801847b9304ec`, file SHA-256
`73e9cc1b19008236657a70655ca2b2b691e2a538ec90778d9f7208c85bd9771d`.
It introduces no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw offset or address, aliasing trick, undefined behavior,
byte forcing, object patch, compiler exception, or compiler-option change.
The two inherited upstream float-bit extraction expressions were not part of
this ownership candidate and were left unchanged rather than silently folding
a second source hypothesis into the shot.

The real compiler first received one `/Zs` parse-only invocation, which passed
without changing the baseline object. Exactly one ordinary Ninja production
edge then compiled `build/base/source/bitmaps/libtiff/tif_dirwrite.obj`; there
was no retry, spelling adjustment, sweep, tuning pass, direct CL compile, or
second code-producing candidate. The immutable ignored first-shot object is
`build/audit/tif_dirwrite_static_auth_first_shot.obj`, raw SHA-256
`3df1802167e80581eb5c9029f6b747cdf922aaca1f354cffcd76ddb3c49de6f9`.

## Strict function result

Hardened `tools/coff_compare.py` proves all fourteen functions equal in
padded bytes and ordered semantic relocations. Normalized bytes zero only
relocation payloads; destinations, types, addresses, addends, symbol
ownership, section flags, and padded extents are independently equal.

| Function | Meaningful | Padded | Relocs | Normalized SHA-256 |
|---|---:|---:|---:|---|
| `_TIFFSetupShortLong` | 71 | 80 | 0 | `67a8a0cb2c8819ade306c07797343ea4084f6268556d8548ae65500736374055` |
| `_TIFFWriteData` | 132 | 144 | 9 | `82160ac720fecfa95a6157d89952487f8757f9b038875de05f2b408d4cf940e8` |
| `_TIFFLinkDirectory` | 361 | 368 | 18 | `aead5f63c387f18caeba013e0c2076969c8a99c883a0ed035e33f77e5618d760` |
| `_TIFFWriteRational` | 134 | 144 | 8 | `243e72aa46d01a99614fba4b456eb400b58a4a1575b09216681481841a0327d6` |
| `_TIFFWriteShortTable` | 114 | 128 | 2 | `f0068ca4fa261d823700aeae274a864e0931a49abdb5f9d7b5bdf17a9c586047` |
| `_TIFFWriteString` | 66 | 80 | 3 | `ddd918e4e80b1f75715d00a4159871b7d41d14cef5c023836ccdc9415be3cdeb` |
| `_TIFFWriteShortArray` | 115 | 128 | 1 | `4175fd50bd828481810b9cf6b9eb923e9164ea3acf772c24f167200a7a764596` |
| `_TIFFWriteLongArray` | 51 | 64 | 1 | `10f2a50d5e57c060834e73f91439d24131b9a5614590806c1c1eade3ce1336c3` |
| `_TIFFWriteRationalArray` | 143 | 144 | 8 | `e73a1566b46fd8abc4bbea20127bd74231e6bbc541e158d9ba469c51602bc40f` |
| `_TIFFWriteFloatArray` | 51 | 64 | 1 | `10f2a50d5e57c060834e73f91439d24131b9a5614590806c1c1eade3ce1336c3` |
| `_TIFFWriteNormalTag` | 800 | 800 | 30 | `12e7519afe0ee92ce3f99e90d7713766a2c935620e2cf8f6aff1d383aa54bf38` |
| `_TIFFWritePerSampleShorts` | 192 | 192 | 2 | `89e725ae1279ceb3363c9dd6a3c7254a062db01916cc4be9d5880668bc813ab0` |
| `_TIFFSetupShortPair` | 101 | 112 | 1 | `3372e4c118e5f1d1cee8d48f53c89881962fc4cede988d4f80ee5673cf72c701` |
| `_TIFFWriteDirectory` | 1,752 | 1,760 | 56 | `f9286a95dd205c8da2eb3e9230a783327273e91bafe130fa88235cf2aa452cb8` |
| **Total** | **4,083** | **4,208** | **140** | **14/14 strict exact** |

All 606 logical non-code bytes remain exact: 492 reported `.rdata` bytes,
110 `.data` bytes, and four BSS bytes. The 110-byte `_data_002b8688` owner has
normalized SHA-256
`86844dbd6a03d2bbdbb4b06984dac301baaa23e0aad59457c9fee2fe43734b72`.
The authenticated four-byte `_dataoff` owner has zero relocations and
normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
Every one of the fourteen target `.rdata` owners independently compares
strict exact; none carries a relocation.

Objdiff itself reports two false negatives because its internal jump-table
relocation encoding differs from the hardened semantic resolver:
`_TIFFWriteNormalTag` at 96.815285% and `_TIFFWriteDirectory` at 90.69021%.
The additive semantic records accept only these two functions, after the
hardened comparator proved respectively 800 padded bytes / 30 semantic
relocations and 1,760 padded bytes / 56 semantic relocations exact. They are
presentation exceptions, not byte or relocation waivers. The unit is now
`Matching` and complete at 4,083/4,083 code plus 606/606 data.

## Repository-wide validation

The full `halobetacache_build`, `libcmt_build`, ordinary progress, and semantic
progress gates pass. The final reports have SHA-256
`fb8e5e5e5f5ce4137c051284b865b91af8907e05b9f68596390830e518ddd80e`
(`build/report.json`) and
`106d8d0da83c74c99f0ec23342467c407c8f43796b8c3091bc9c0bfa74a1c39b`
(`build/semantic_report.json`). The semantic audit scanned 470 units and
4,881 functions: 4,765 strict semantic exact, 131 hidden exact, 4,775
accepted exact, one ordinary-only/structural record, zero rejected records,
and zero unit errors.

Object admission reports zero candidates, contradictions, revocations, or
revoked Matching units. Parked-function audit reports 12 active, zero stale,
and zero invalid; this object adds no park. The complete tooling suite reports
205 passed. `git diff --check` is clean, and `git status --short -- tools`
shows no unpublished tool change. The closeout is therefore a source/config
recovery with no tool, binary, generated report, audit artifact, or split
object committed.
