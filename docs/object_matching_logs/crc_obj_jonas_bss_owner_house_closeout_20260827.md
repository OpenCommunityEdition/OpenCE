# `crc.obj` BSS-owner and house-style closeout (2026-08-27)

## Result

`source/memory/crc.obj` advances from 2/3 to 3/3 strict-exact functions and
from 80/208 to 208/208 strict padded code bytes. The remaining function was
already byte-identical to January. Its four BSS relocations differed only
because the candidate named the aggregate `_crc_state`, while the January
object owns the same 1,025-byte aggregate through `_bss_00456220`.

The retained source restores that natural owner and keeps the readable name as
a translation-unit alias:

```c
struct crc_globals bss_00456220;
#define crc_state bss_00456220
```

No address, offset, extra payload, linker/compiler change, synthetic anchor,
object patch, or comparator/admission exception is involved.

## Authority and bounded evidence

- Published base: `0f4f9c347b99b5856febcb5d0ec4356514e833b5`.
- January object SHA-256:
  `7d066c36a597def9a2e0156d3193728caf2c424be5204cfb54c07aac97cc7103`.
- Retained `crc.c` SHA-256:
  `2507f9bb2ebc694839b3967733e2292238e524f9ac8e42518d06991602e1c2b1`.
- Retained `crc.h` SHA-256:
  `c8e8274fcf5e6c0c8fb0152ef0914e48f39044aa160a8c8f0505907bd07ea77d`.
- First-shot scratch object SHA-256:
  `05010e49c9c502753ba5a32ba435717ee70e65697f09bcfd946f70cf65000a8b`.
- Compiler: the pinned XDK 3911 VC7 compiler and unchanged generated flags.

There was no prior `crc.obj` experiment ledger or alternative Claude source
body to transplant. The target relocation inventory itself provides the
missing ownership fact: offsets `+0x2A`, `+0x33`, `+0x3E`, and `+0x5F` name
`_bss_00456220` with addends 1,024, 0, 1,024, and 0. The pre-edit candidate
used the same addresses, types, addends, bytes, and BSS layout but named
`_crc_state`.

## Exact functions and owned data

| Symbol | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_crc_new` | 16 | 0 | `e3d27bf7f5aaa1e7f59a5adeb31d265acb1030a7e93368fb6f7c4171422cf713` |
| `_code_001088e0` | 64 | 0 | `a4b863bff24b1f220eaf4f2732125741dd27aacd866d358f0c77f51266e28c41` |
| `_crc_checksum_buffer` | 128 | 9 | `3926c1e6efc7c4e8870b487330a42ac200a8d05d5d43650d62d00c135b29ff73` |
| `_bss_00456220` | 1,025 | 0 | `c55b90509b8cb9bac53fbdddfc93d4e572685c509f1218423c43a5d6013bbd48` |
| `buffer_size>=0` assertion string | 15 | 0 | `940d8cde4884a72ab8d8844ec24275d2a343733dfd9c79df4ac38fe0bf65721d` |
| source-path assertion string | 28 | 0 | `5503e6ebd6aeef2e407863bf802d9c87820511443cc66a93529dd04ca621c69f` |

The one frozen scratch compile accepted all three functions and all three
owned data sections strict exact. Both previously exact siblings remained
exact.

## House-style audit

All active definitions and public declarations now put every parameter on its
own line. Every void definition has an explicit terminal `return;`. The
existing packing and BSS-segment directives are unchanged and no optimizer
pragma was introduced. This unit has no tag or object access.

The retained source contains no assembly implementation, `register`,
`volatile`, intrinsic, attribute, optimizer barrier, raw address,
pointer/integer reconstruction, type pun, undefined behavior, byte forcing,
object patch, comparator exception, or compiler/flag substitution.

## Validation

The frozen production compile ran exactly once:

```text
[1/1] CL build\base\source\memory\crc.obj
crc.c
```

The production object reproduces the scratch result: all three functions, the
1,025-byte BSS aggregate, and both assertion-string sections are strict exact.
Repository-wide validation then passed:

- complete Ninja split/report/semantic/progress pipeline: pass;
- strict board: 269/619 exact objects, 4,342/8,246 exact functions, and
  563,287/1,922,669 padded code bytes;
- `crc.obj`: 3/3 functions and 208/208 padded code bytes;
- semantic audit: 470 units, 4,474 functions evaluated, 4,351 semantic exact,
  4,378 accepted exact, and zero unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- full tooling suite: 194 passed;
- `build/report.json` SHA-256:
  `bb838270ad8df6971c87f794b6c7af66709809185255551f76b2ba074fab177f`;
- `build/semantic_report.json` SHA-256:
  `aafbb9eec35e78a0111d6b7e094d6b46f9d62e7afaae21ffea374802d0f16c74`.

`config/config.json` already labeled the unit `Matching`; this wave repairs
that pre-existing strict-admission inconsistency, so the configured 377/833
linked-file count does not change. No configuration edit was needed.

The requested tools-publication audit also remains clean: local HEAD and the
published `jonas/exact-pilots` branch each contain 121 tracked `tools/**`
paths, with no path difference and no local tool edit.
