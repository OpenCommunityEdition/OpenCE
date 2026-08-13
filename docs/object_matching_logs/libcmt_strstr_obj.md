# `libs/libcmt/strstr.obj` provenance and park ledger

## Plain-English behavior

`strstr` searches a byte string for the first occurrence of another byte
string. The January implementation includes a special case for a one-byte
needle: instead of running the general substring loop, it transfers directly
into the middle of the separately assembled `strchr` routine.

That last detail is decisive. This is not C that merely happens to compile
unusually. It is a hand-written Microsoft Macro Assembler object whose source
file and assembler version survive in the shipped XDK object.

## Status

Parked as original, vendored Microsoft CRT assembly. No production C file was
retained and `config/config.json` remains `MISSING`. The object receives no
matching credit under the project's readable-C-only policy.

## January target inventory

- Runtime section: one 128-byte, 16-byte-aligned `.text` section
- Defined runtime entry point: `_strstr` at offset zero
- Runtime relocations: one `IMAGE_REL_I386_REL32` relocation at `+0x6F`
  to `___from_strstr_to_strchr`, addend zero
- Owned `.data`, `.bss`, or `.rdata`: none
- Extra symbol: `___from_strstr_to_strchr` is undefined here and is defined by
  the separate `strchr.obj` at offset `+0x16` inside its `.text` section

The raw target sequence around the transfer is:

```text
33 C0 5E 5B 5F 8A C2 E9 00 00 00 00
```

It clears the result register, restores the saved registers, moves the first
needle byte into `AL`, and tail-jumps through the relocation into the internal
`strchr` entry. This is not a call to the public `_strchr` function boundary.

## Authoritative XDK 3911 provenance

The locally installed XDK 3911 archive is:

```text
C:\tmp\xdk3911_extract\XDK\xbox\lib\libcmt.lib
SHA-256: 60AA4C87C97EEB6662B31D9D37CF3CABA1AFABAEC72C47B090E7A0CE8B632F55
```

Its library index contains `obj\i386\strstr.obj` and
`obj\i386\strchr.obj`. The extracted `strstr.obj` has timestamp
2001-08-11 11:19:36, and its COFF `.file` record is
`..\i386\strstr.asm`. Its CodeView data identifies:

```text
Microsoft (R) Macro Assembler Version 6.15.8803
```

The companion member is independently identified as `..\i386\strchr.asm` by
the same assembler version and defines `___from_strstr_to_strchr` at `+0x16`.
This proves both the implementation language and the cross-object internal
label; no web source or cross-build inference is needed.

The extracted XDK `strstr.obj` itself has SHA-256
`635D8ED9D2ADCF994CC6BBA8F64AA72C449E2F023329B6B56CECEECA32567ADC`.
Comparing its `_strstr` runtime section to the January split target gives:

- size: `128/128`
- relocations: `1/1`
- normalized SHA-256, both sides:
  `9b149ba7733ac0566a87a6c361389c8fbfe61b51b0fc881f638975ac0b003797`
- relocation address/type/destination/addend: identical
- `section_infos_equal = true`

The shipped member also contains debug records and a zero-length `.data`
section. Those are non-runtime assembler bookkeeping; the January split
object correctly owns only the 128 runtime text bytes.

## Measured readable-C experiments

Both experiments used XDK 3911 CL 13.00.9254.1 with the libcmt object's
unchanged `/O1 /Gy` flags. They were disposable fixtures, not production
source.

| ID | Readable-C source shape | Size / relocs | Normalized SHA / decisive result |
|---|---|---:|---|
| C01 | Natural nested candidate/needle comparison loop, including empty-needle handling | `86 / 0` | `9bea69da7c64d4e8680c6718ce4fa19af4d53b0b613f6551c2f863b4a8dd863e`; 42 bytes short and no relocation |
| C02 | Same natural loop with a one-character fast path calling public `strchr` | `107 / 1` | `5ad8820f5248eb515e6d307f471bf98f605beb57189183041d352d5a31395da0`; relocation is at `+0x1F` to `_strchr`, not at `+0x6F` to the internal label |

Declaring `___from_strstr_to_strchr` as a C function would not recover source;
it would invent a false function boundary and ABI for a label proven to be in
the middle of `strchr.asm`. Inline assembly, an external assembler file, or a
byte blob could reproduce the vendor object, but all three are outside the
current readable-C exact-match standard. Further C spelling experiments would
therefore optimize around the wrong source language and are not justified.

## Validation of the docs-only decision

- Full `halobetacache_build` and `libcmt_build`: successful
- Progress and semantic reports regenerated without error
- Tooling unit tests: `179/179` passed
- Production/config diff: empty; only this ledger is committed
- Because a `MISSING` unit has no rebuilt base object, it cannot itself be a
  regression-gate baseline. A clean snapshot/check of the adjacent accepted
  `libs/libcmt/wcscoll` unit is used after this documentation commit to prove
  the lane has not disturbed generated build or ownership evidence.

## Reopen criterion

Reopen only if the project explicitly adopts a reviewed policy for preserving
original vendor assembly members (preferably by importing the authenticated
XDK member or original assembly source with provenance), or if the standard is
changed to permit such non-C runtime objects. Do not reopen for declaration,
branch, or register-allocation sweeps.

## House/Berth audit

The final repository change is documentation only. No assembly, `volatile`,
forced inline, undefined behavior, byte patching, codegen cast, alignment
coercion, compiler-flag change, comparator exception, invented alias, or raw
object/tag access was added to production. The target behavior is ordinary
CRT behavior rather than a confirmed engine bug, so no bug-preservation
comment is appropriate.
