# Public object-owned BSS: two initializer corrections

Baseline: `ebd24ae1e7282b8019989492eea82b310d51360c`.
Scope: rules 47/48 a-ext only; no c-layout admission, declaration-census batch,
header changes, function-linkage corrections or new object certification.

## Correct the packet inventory first

RF-BQ P12b contains **one** `= FALSE` initializer, not two. Its other item is
the public function `_cache_copy_FileIOCompletionRoutine@12`. P48b contains
one `= NULL` initializer and nine unrelated function-storage metadata fixes.
The three a-ext symbols therefore comprise two variables and one function.
Do not apply either donor patch wholesale under an initializer ruling.

This packet changes exactly:

- `decompressor_print_timing`: private Boolean to public, explicitly zero
  initialized; remove only its `symbols.json` `static` flag at 5038064.
- `global_debug_key_down`: private pointer to public, explicitly null
  initialized; its target symbol was already correctly public.

Both initializer spellings are inferred from positive linkage/allocation
evidence under the frozen compiler, not recovered declaration text. The
existing types, values, offsets and references are retained. Comments replace
existing lines, preserving all source line numbers.

The callback and nine debug-function storage discrepancies remain separate,
disclosed issues. Neither existing Matching status is newly awarded here.

## Independent January evidence

Inputs:

- `cachebeta.exe` SHA256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- `cachebeta.pdb` SHA256
  `8480f0c44fc7b5acba5775c02053d1a62c6794ab8d646661106985cbe46d7bc5`.

The original PDB's raw public-symbol stream 834 and DBI contributions give:

| Variable | Public record offset | Image RVA | DBI contribution | Owner |
| --- | ---: | --- | --- | --- |
| `_decompressor_print_timing` | 216960 | `0x4cdff0` | RVA `0x4cd330`, size `0xcc1`, flags `0xc0400080`; variable at `+0xcc0` | Module 33, `cache_files_decompress_windows.obj` |
| `_global_debug_key_down` | 590468 | `0x455748` | RVA `0x455748`, size 4, flags `0xc0300080`; variable at `+0` | Module 233, `debug_keys.obj` |

Both are `S_PUB32_ST` (`0x1009`) records. The PE merges their storage into its
`.data` image section, but the original contribution flags prove input BSS.
These are direct per-object contributions, not ownership guesses from the
synthetic COMMON pool. The pointer's four linked-image bytes are zero.

The September 25, 2001 map separately identifies both names and owners in
Publics by Value (lines 18945 and 18898). The August 15 map also identifies the
pointer (line 17836), but does not contain the timing name. Do not claim that
map corroborates the Boolean.

The generic old-PDB symbol walker has an incompatible record-tail limitation.
The required records were validated directly (or on valid boundaries before
that tail); no absence or whole-stream completeness claim depends on it.
Raw contribution/module records and selected record hex are preserved in the
local evidence receipts.

## Compiler control and boundary of this ruling

A stock VC7 `/O2 /Oy- /DDEBUG /Dxbox /W3` control distinguishes:

| Source form | COFF allocation |
| --- | --- |
| `extern boolean name;`, referenced | Undefined external: section 0, value 0, storage 2 |
| `boolean name;` | Tentative COMMON: section 0, value 1, storage 2 |
| `boolean name = 0;` | Allocated owner BSS, storage 2 |
| `static boolean name;`, referenced | Allocated owner BSS, storage 3 |

Thus a bare **extern declaration does not become COMMON**. An external-linkage
tentative definition does under this compiler. The explicit initializers
preserve the proven owner allocation when restoring public linkage.

This does not authorize zero initializers merely because static declaration
order reproduces a layout. The separate c-layout proposals remain held;
the earlier unused-static emission evidence is not established by order alone.

## Fresh isolated and integrated checks

Each variable was compiled alone from the unchanged baseline and compared
independently before the minimal packet was applied:

- Decompressor: **46/46** exact functions; all **116** nondebug sections retain
  payload, size, relocations, flags/alignment and auxiliary section metadata.
- Debug keys: **12/12** exact functions; all **26** nondebug sections retain
  the same properties.
- The sole named-definition change in each object is storage **3 -> 2**, at
  the original BSS offset. No new section, COMMON, undefined symbol or helper.
- Raw section/symbol-table ordering changes; this is not a claim that complete
  COFF files are byte-identical. Section identities and resolved references
  are independently compared rather than relying on ordinal equality.
- Stock warnings remain zero; explicit `/W3` controls each retain the same
  twelve inherited warnings. Integrated objects match the isolated candidates.
- Provider/reference censuses cover 622 current compiled objects. Each name
  has exactly one occurrence/provider, in its proven owner; no duplicate,
  COMMON or cross-TU consumer. Debug keys additionally checks all 833 target
  objects and all five original-image HIGHLOW references. No unexecuted
  provider-pair probe is claimed as a link receipt, and no whole-program link
  or runtime certificate is claimed.

After target regeneration, all 142 affected nondebug sections are unchanged
from the baseline. Only the intended two storage classes differ in rebuilt
objects; only the Boolean's storage class differs in the regenerated targets.

Full final battery:

- `ninja all_source progress build/report.json`: PASS.
- Stable sweep: **8252 rows, 7641 exact; 0 gains, 0 regressions**.
- Parks: **71 active, 0 stale, 0 invalid**, entire report unchanged.
- Admission: **12 candidates, 0 contradicted, 1 rejected, 0 revoked**, unchanged.
- Fake-match scan: **26 inherited findings**, unchanged.
- Pytest: **1161 passed, 5 skipped, 26 subtests passed**.
- `git diff --check`: clean. Inherited README and research work preserved.
- Raw progress and semantic progress reports are JSON-identical before/after.

Credit is **zero**: Halo remains **1,598,242 / 1,770,166 meaningful code bytes**,
**7469 / 7574 credited functions**, **390 / 468 complete objects**, and
**2,588,903 / 3,923,451 data bytes**. This is a storage/source correction.

Local receipts: `scratch/astra_public_bss_20260927/`, including the scope review,
separate decompressor/debug-key primary audits and sealed A/B compiles, compiler
control, baseline/after stable snapshots, full gate logs, and the coordinator's
`final_verify.json`. No donor tree was modified and no push is part of this packet.
