# `libs/libcmt/frnd.obj` provenance park ledger

This atomic lane starts from campaign commit `8447e220`. At lane start
`libs/libcmt/frnd.c` was config index 639 with status `MISSING`. The object was
absent from `config/parked.json`, every Git ref and worktree, and all active
agent ownership lists. The January split target is present. Production source
and config remain untouched because primary provenance proves the function is
implemented with inline x87 assembly.

## Plain-English behavior

`_frnd` rounds a double to an integral double using the x87 control word's
current rounding mode. It deliberately does not mean only floor, ceiling,
truncate, or round-to-nearest: the result depends on the caller's active x87
rounding-control bits. The x87 operation also has its own NaN, infinity,
denormal, and floating-point-exception behavior.

## January inventory

The target owns one external COMDAT function and no runtime data:

- `_frnd`: `0x13` bytes, zero relocations, normalized SHA-256
  `b2acfb60d9432e4ad64781ac801918ada55d9c22650b1504c4ee039ed85da31f`;
- `.text` flags `0x60101020`, pick-no-duplicates, one-byte alignment;
- no target-owned `.rdata`, `.data`, or `.bss`;
- no aliases.

Its instruction sequence allocates an eight-byte temporary, loads the double
argument, executes `frndint`, stores the rounded value to the temporary,
reloads that value as the x87 return, releases the temporary, and returns.

## Primary XDK 3911 provenance

The locally archived XDK 3911 `libcmt.lib` has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\frnd.obj` member has SHA-256
`e06cbd3b43d3f0c124761dc0bdda28d4c8cd1807f0aa41ea329e90c84a021378`
and records:

- compiler id `0x001c2426`;
- source path
  `d:\xbox-finalfre\private\sdktools\crt\fpw32\tran\i386\frnd.c`;
- public prototype evidence for `_frnd`, one double argument named `x`, and a
  double stack local named `result`;
- source line records spanning lines 31 through 45.

Most importantly, the line table maps function offsets to consecutive source
lines as follows:

| Offset | Source line | Instruction |
|---:|---:|---|
| `+0x02` | 36 | `fld x` |
| `+0x06` | 37 | `frndint` |
| `+0x08` | 38 | `fstp result` |
| `+0x0c` | 44 | compiler reload of `result` for the return |
| `+0x10` | 45 | epilogue/return |

Those three consecutive source-line mappings, together with the named `x` and
`result` debug records, identify the historical body as an inline-assembly
block embedded in a `.c` source file. This is stronger evidence than the `.c`
suffix alone and prevents incorrectly classifying the unit as natural C.

The XDK member's runtime function passes `section_infos_equal` directly
against January: `0x13/0x13` bytes, zero relocations, the same normalized hash,
and identical COMDAT ownership/selection/alignment. Its extra `.debug$S`,
`.debug$F`, and `.debug$T` sections are compiler metadata, not runtime data.

## Experiment matrix

| ID | Evidence or source shape | Size T/B | Relocs T/B | Hash/result | Decision |
|---|---|---:|---:|---|---|
| P01 | Compare exact XDK archive member to January | `0x13/0x13` | `0/0` | strict exact, `b2acfb60...da31f` | accepted as provenance only |
| S01 | Natural readable-C reconstruction | not compiled | not compiled | no source-faithful C operation exposes current-mode `frndint` under this compiler | rejected before production edit |
| S02 | Historical inline x87 assembly body | would reproduce the archive | `0/0` | prohibited by campaign policy | never introduced |

No production source shape was attempted. That is intentional, not missing
work: the task requires readable C only, while the recovered implementation is
inline assembly. A cast through an integer would use conversion instructions,
would fail for values outside the integer range, and would change floating-
point exceptions. `floor`, `ceil`, and truncation implement fixed modes rather
than the current control-word mode and introduce calls or additional control-
word manipulation. None is a semantic or byte-exact substitute.

## Disposition and reopen criteria

`frnd.obj` remains `MISSING` and receives no exact-match credit. No source,
config status, parked ledger, compiler flags, comparator rule, or semantic
exception is changed. The unit is parked by documentation as **proven
vendored inline assembly**.

The unchanged `halobetacache_build` and `libcmt_build` pass. The progress
report passes at 310/833 objects overall, 267/468 Halo objects, and 43/212
libcmt objects; none of that credit comes from `frnd`. The semantic audit
scans 413 units, accepts 3,389 exact functions, and reports zero unit errors.
All 179 tooling tests pass. A production forced rebuild is deliberately not
possible because no policy-compliant candidate source/object exists; the
reproducible strict check is the independently extracted XDK member against
the January target, which passes byte and ownership equality as recorded
above. The regression tool correctly rejects `libs/libcmt/frnd` as an unknown
objdiff unit because a `MISSING` object has no candidate build edge. Its
campaign-wide `--all` mode is independently blocked by the existing
space-containing `source/saved films/saved_films` Ninja-edge parser issue.
As an executable sentinel, a post-commit snapshot/check of the adjacent exact
`libs/libcmt/fp8` unit retains `_setdefaultprecision` with no failures or
warnings. Together with the full unchanged builds and the one-file Git diff,
this proves the documentation-only lane changes no production artifact
without pretending that `frnd` has a candidate object.

Reopen only if campaign policy explicitly admits the verbatim Microsoft/XDK
inline-assembly source with appropriate licensing/provenance handling, or if
the exact compiler is independently demonstrated to emit `frndint` from a
readable, defined-behavior C construct with the same current-rounding-mode and
exception semantics. Similar-looking `floor`, cast, or modern `rint` source is
not sufficient.

## House/Berth and bug audit

The final repository change is documentation only. It does not disguise
assembly as C, use `volatile`, force inlining, rely on undefined behavior,
patch bytes, add opaque casts, alter alignment/flags, or weaken comparison.
There is no original-game bug: current-rounding-mode behavior is the intended
CRT operation. The explicit stop preserves both readability and the project's
strict provenance standard.
