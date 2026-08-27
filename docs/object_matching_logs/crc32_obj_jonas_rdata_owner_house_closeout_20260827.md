# `crc32.obj` Jonas rdata-owner and house-style closeout (2026-08-27)

This ledger records the strict whole-object closeout of
`source/memory/zlib/crc32.obj`. Both function payloads and the complete
1,024-byte CRC table were already normalized-equal. Strict credit was blocked
only because candidate relocations named local `_crc_table`, while January owns
the same table through external `_rdata_0027b378`.

The unit advances from 0/2 to 2/2 functions and from 0/320 to 320/320 padded
exact bytes. The repository advances from 263/619 to 264/619 fully strict
objects, from 4,319 to 4,321 strict functions, and from 558,679 to 558,999 strict
padded bytes.

## Authority and bounded scope

- Published base: `f9256add2cccab974e7b92f4aa893cb77656cc15`.
- Baseline source blob: `a91101a81c6ae58913d5f3266e8da4ff75eda936`.
- Retained source blob before commit: `178bef79b2764d90eedc9caf2789cd2a5f770ee0`.
- Retained source SHA-256:
  `6f0b878c566f08d39a09be84f2417c95f185ce0d45cf338153fa385a2a90adf7`.
- January object SHA-256:
  `9f5bce6c43a7c2d6264b52a692e4f0b978fc02422d72d196bfd4694ef3caaed4`.
- Sole candidate object SHA-256:
  `00abc84cf5dc671d0469008350239527dcb19d565cbb7d15d281f1c75cb73f59`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged production edge.

January COFF is authoritative for padded bytes, relocations, addends, symbol
identity, and table ownership. The checked-in zlib implementation is the
readable behavior and source-topology authority. The campaign's current
Claude/AGENTS house and tooling rules were applied to both active and conditional
source. Only `crc32.c` and this ledger change; no header, config, compiler flag,
comparator, admission rule, semantic allowlist, parked entry, or tool changes.

## Natural table ownership recovery

The non-dynamic configuration's existing table is declared as the external
const array `rdata_0027b378`, and the readable upstream name remains available
inside the implementation through a preprocessor alias:

```c
const uLongf rdata_0027b378[256] = { /* existing values */ };
#define crc_table rdata_0027b378
```

This restores January's section owner at offset zero without introducing an
address, offset, extra payload, aliasing violation, or linker/compiler change.
The complete table is strict exact:

| Owner | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rdata_0027b378` | 1,024 | 0 | `12f3e0576d447eb37b36d82ba0c1c5481b8f0d12fdc70347ce4a076b229d4c86` |

All ten function relocations now name that owner: one from `get_crc_table` and
nine from the unrolled `crc32` implementation.

## Exact functions

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_get_crc_table` | 16 | 1 | `9f90b76585a2e9d12439da053c9d502d27587d74cc7441c902dd9c68c789c2fc` |
| `_crc32` | 304 | 9 | `08998347ed0236b63263b8292fede3c08ec76dc1bd7aec3976e4e889f188b783` |

Every padded byte and every relocation address, type, destination, owner, and
addend matches.

## House style and frozen compile

The same frozen edit converts `get_crc_table` to the required multiline `(void)`
form and converts `crc32` from K&R syntax to an ordinary C89 definition with one
parameter per line. The conditional `make_crc_table` definition receives the
same no-argument form and an explicit terminal `return;`, so inactive source
does not retain house debt.

Exactly one candidate compile ran:

```text
[1/1] CL build\base\source\memory\zlib\crc32.obj
crc32.c
```

It was immediately preserved at
`build/audit/crc32_owner_house_first_shot_20260827.obj`. Direct hardened
comparison accepted both functions and the complete table section; no tuning,
retry, or retained replay occurred.

The retained source contains no assembly implementation, `register`,
`volatile`, pragma, intrinsic, attribute, optimizer barrier, raw address,
pointer/integer reconstruction, type pun, undefined behavior, synthetic anchor,
byte forcing, object patch, comparator/admission exception, or compiler/flag
substitution.

## Full validation

- Halo and libcmt complete build targets: pass.
- Split, ordinary report, semantic report, and strict progress: pass.
- Strict board: 264/619 exact objects, 4,321/8,246 functions, and
  558,999/1,922,669 padded bytes.
- `crc32.obj`: 2/2 functions and 320/320 padded bytes; absent from the
  remaining-work list.
- Semantic audit: 470 units, 4,467 functions evaluated, 4,330 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,374 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `e75cbef4f35f453e1db52db538c799ed2febefc2263ca1cd51b58fb7ca444b15`.
- `build/semantic_report.json` SHA-256:
  `24093448613c677759c417b75e4f6501a6bb6eff5531d9b2752d939b34eb4c64`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable pytest-cache
  warning.
- Tools publication audit: 121 tracked paths and zero changed or untracked
  `tools/**` paths.
- `git diff --check`: clean apart from Git's line-ending notice.

`config/config.json` already labels this unit `Matching`; the strict evidence now
justifies that existing label.
