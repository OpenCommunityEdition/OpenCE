# `debug_memory.obj` Codex checkpoint

This is a Codex-owned checkpoint ledger for
`source/cseries/debug_memory.obj`. It records a strict partial reconstruction;
the object remains `NonMatching`, and no pre-existing or Claude-authored
Markdown was modified.

## Scope and provenance

- Integration base: `018aa03f36fc0deb3e9e47c73adc4133c9c6a165`.
- Source leads were isolated historical commits `9100401a`
  (`debug_dump_memory_for_file`), `66faa534`
  (`debug_dump_memory_by_file`), `fbb90eb9` (`debug_free`), and `666b1f03`
  (the final target format-string correction).
- Only the relevant readable C was reconstructed. No donor configuration,
  parked entry, symbols file, frozen-object path, or Markdown was imported.
- The untouched base rebuilt at 10/18 strict functions and 848 padded exact
  code bytes. The retained source rebuilds at 13/18 and 1,792 padded exact
  bytes: a gain of three functions and 944 padded bytes.

## Retained reconstruction

`debug_dump_memory_for_file` walks the allocation list, lazily opens the heap
dump, prints matching allocations, and reports the selected total.
`debug_dump_memory_by_file` aggregates at most 512 source files in a typed
`file_pointer_totals` array, sorts by total allocation size, and prints the
per-file and global summaries. The comparison callback now has the standard
`qsort` signature and converts each `void const *` argument once to the typed
record; this is type-safe and still emits the exact 32-byte callback.

`debug_free` became exact after the trailing signature was addressed from the
end of the typed header, `(byte *)(header + 1) + header->size`, rather than
from the user pointer. Public declarations were reformatted to the repository
parameter style without changing their ABIs.

The retained source contains no assembly, volatile scheduling device,
force-inline annotation, optimizer pragma, barrier, undefined type-punning, or
byte-forcing construct. Void functions end with an explicit `return;` and the
new typed records are accessed through their declared types.

## Strict function evidence

The January target and a clean XDK 3911 rebuild agree in padded bytes,
relocation count and identity, and normalized SHA-256 for all 13 accepted
functions.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_debug_memory_manager_initialize` | 48 | 7 | `a1f80c7dc4894b3ee5295743d8807b7a6231182bbbd76d3239b5e5479812d7ba` |
| `_code_0007ccf0` | 80 | 7 | `81fe5aab6b56290e6e3d782b2decb1d10f3bd000edea7cf43fb74aa6df728681` |
| `_code_0007cd40` | 80 | 6 | `b22ae665fb99b8c7871620ecd1bbaec8db267451614f7f870f77ffcfbc499d54` |
| `_code_0007cd90` | 48 | 2 | `6d5b2166f0614fcdce5609b8596b089355868b79956b072c010c12a1f2cd6e95` |
| `_code_0007cdc0` | 32 | 0 | `181b8a9ae78716e31e3b8f516f06803ec0e73c15327a802b6f34f5b0f395b254` |
| `_check_memory_status` | 80 | 3 | `56299d9c9a4295e90d2fbbd20a9bbc8e315ac9848eea9a50563eeeefc1be746f` |
| `_local_random` | 16 | 2 | `a8ad3cb30f329f39f3193d4112f79d5983d27e7d41b1c99b5e0b4cd98e555bd0` |
| `_code_0007ce40` | 272 | 26 | `b15c01fb494f74d748a3f18a8359825a67599c8febad4ce4eff4a1a8bcb20d9b` |
| `_debug_check_memory` | 176 | 15 | `146eec163623ea843eb26a91a44b8e5a88eb6f64975fdf19d7e4e6215070a88f` |
| `_debug_dump_memory_for_file` | 224 | 18 | `90e9624632f2c9fba27c6ffda2dbb667bdfb498aa631f098d5fdc8ce433682be` |
| `_debug_dump_memory_by_file` | 512 | 24 | `8d562f63f0cd2f6a2d7ea7eccf42ce28bae6d79c00973dc9fd34f9a4fdae09d2` |
| `_debug_free` | 208 | 18 | `4cbca9054f471b691aefd102571cc980b71f7e4cdb3617db5df6c3ebcfcf60aa` |
| `_debug_dump_memory` | 16 | 1 | `9bb7c3e3df51e01156b5c7360bca124d4b6dd23f69818f849dbe584775065f7b` |

Totals: 1,726 meaningful bytes, 1,792 padded bytes, and 129 exact
relocations. A direct base-before/base-after comparison proved that every
previously emitted function is byte-identical to the integration base except
for the intended `_debug_free` repair; the two dump routines are new emitted
definitions.

## Honest residual inventory

| Function | Target/candidate padded bytes | Target/candidate relocations |
| --- | ---: | ---: |
| `_code_0007cf50` | 112 / 96 | 8 / 8 |
| `_code_0007cfc0` | 160 / 144 | 11 / 10 |
| `_code_0007d060` | 224 / 208 | 14 / 14 |
| `_debug_malloc` | 272 / 272 | 20 / 20 |
| `_debug_realloc` | 416 / 400 | 25 / 25 |

These five definitions remain nonexact and receive no credit. The
configuration therefore remains `NonMatching`.

## Data and ownership audit

All 27 target-owned runtime RDATA/DATA sections are strict-exact by section
owner, logical size, flags, alignment, COMDAT selection, payload hash, and
relocation identity: 1,073/1,073 logical bytes, with no target-only or
nonexact target-owned section. The ordinary linked report records the same
object as 1,120/1,120 padded data bytes.

The candidate additionally emits the ordinary `.drectve` section, three
unreferenced SELECT_ANY XDK table COMDATs
(`_D3DTEXTUREDIRECTENCODE`, `_D3DSIMPLERENDERSTATEENCODE`, and
`_D3DPRIMITIVETOVERTEXCOUNT`), and one unreferenced SELECT_ANY `"file"`
string COMDAT. These are candidate-only discardable sections, are not claimed
as target ownership, and do not replace any target-owned section.

## Reproducibility and gates

- Source SHA-256: `14710f8dea25bcf2459aa65d1e266ee50020673ac303f1bd27fc131690de0486`.
- January target object SHA-256:
  `bcf575ff9e4dc3749ba66beb41991f62985262fccc3c643f06251d14ba942244`.
- Full clean `halobetacache_build` plus `libcmt_build`: 569/569 edges passed.
- Semantic audit: 470 units, 3,688 functions evaluated, 3,546 semantic exact,
  3,628 accepted exact, and zero unit errors.
- Progress: 374/833 complete objects, 3,623/11,060 exact functions,
  438,100/2,198,102 exact code bytes, and 1,802,644/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, zero invalid.
- Tool suite: 179/179 tests passed.
- `git diff --check` passed before commit. A clean post-commit forced-object
  regression snapshot/check is required and recorded in the final handoff.

The five user-frozen large objects are untouched, no tracked file was deleted,
and no Claude-authored Markdown was changed.
