# `data_packets` verifier initialization research

## Verdict

No fully defined, readable-C initialization/carry topology in the bounded
25-variant XDK 3911 matrix reproduces January's `_code_0010a5f0` bytes.

The copied uninitialized control is strict-exact:

- size: 464 / 464 bytes
- relocations: 27 / 27, including semantic relocation identity
- normalized SHA-256:
  `7ecfeee48d0b0bab52bd5008721ab67c724569db3e0dd4ff603d3d18eab5701f`

All 25 fully defined variants retain 464 bytes and 27 relocations, but none
has the target normalized hash.  The ordinary zero-initialization family
compiles to
`5a14c0cf7955e7ab0bfb3ded83af75c603d657e245a6ed27a0110564d3834a49`.

This is not merely a compiler spelling preference.  January's emitted control
flow proves that the missing initialization is observable on a legal input
shape (a first non-end field outside the definition's version range).

## Machine-code proof

Relevant January flow:

```asm
+0x06  mov  eax,[ebp+10h]        ; fields
+0x09  cmp  word ptr [eax],9     ; first field is end?
+0x0d  push esi
+0x0e  mov  esi,eax              ; current field
+0x10  mov  dword ptr [ebp-0Ch],0 ; total_size = 0
...
+0x22  mov  edi,[ebp+10h]        ; EDI receives fields, not a size
...
+0x9c..0xb6                      ; version eligibility tests
         jl/jne +0x150           ; ineligible field bypasses every size case
...
+0x150 mov  edx,[ebp-0Ch]
+0x153 mov  word ptr [esi+8],di  ; reads low word of incoming fields pointer
+0x157 add  edx,edi              ; carries the same indeterminate source value
```

On an eligible field, a switch arm defines `DI` before the join.  On a first
version-ineligible field, no instruction defines a field size: the low word of
the incoming `fields` pointer happens to occupy the allocator's chosen EDI
register and is consumed.  On later ineligible fields, the prior switch
result is carried.  This exactly explains both the C-level uninitialized read
and the binary.

Any defined C implementation must define the first ineligible value.  Under
the project rules, it has only two possibilities:

1. Materialize a legitimate value such as zero.  XDK 3911 keeps the same
   size/relocations but changes register allocation beginning at `+0x09`.
2. Define it as the low bits of the `fields` pointer to mimic the accidental
   register contents.  That is fabricated pointer-derived semantics and a
   cast-for-codegen technique, explicitly outside the allowed source language.

Therefore an exact and policy-compliant source cannot represent this target
path unless the project creates a narrowly documented exception to its UB
ban.  This conclusion is scoped to readable legal C under the existing flags;
it does not claim that arbitrary codegen-forcing C, inline assembly, or a byte
patch is impossible (all are forbidden here).

## Bounded experiment matrix

Every row used CL 13.00.9254.1 with `/O2 /Oy- /DDEBUG /Dxbox`.  Every defined
row is 464 bytes with 27 relocations.  `Diff` counts differing normalized byte
positions; all defined variants first diverge at `+0x09`.

| ID | Fully defined source topology | Hash prefix | Diff | Exact |
|---:|---|---|---:|:---:|
| control | Current uninitialized source (method control only) | `7ecfeee48d0b` | 0 | yes |
| 00 | `total=0; field_size=0;` | `5a14c0cf7955` | 355 | no |
| 01 | `total=0; field_size=total;` | `5a14c0cf7955` | 355 | no |
| 02 | `short field_size=0;` | `5a14c0cf7955` | 355 | no |
| 03 | declaration-init total, then copy into field size | `4c795546f089` | 355 | no |
| 04 | both declaration-initialized, field first | `4c795546f089` | 355 | no |
| 05 | both declaration-initialized, total first | `4c795546f089` | 355 | no |
| 06 | `field_size = total_size = 0` after field binding | `5a14c0cf7955` | 355 | no |
| 07 | same chained assignment before field binding | `4c795546f089` | 355 | no |
| 08 | comma expression: total zero then field copy | `5a14c0cf7955` | 355 | no |
| 09 | field binding + both initializes in one comma expression | `5a14c0cf7955` | 355 | no |
| 10 | zero through constant-true pointer-identity conditional | `5a14c0cf7955` | 355 | no |
| 11 | zero through constant-false pointer inequality | `5a14c0cf7955` | 355 | no |
| 12 | same declaration: total zero, field copy | `4c795546f089` | 355 | no |
| 13 | same declaration: field zero, total copy | `4c795546f089` | 355 | no |
| 14 | total initializer performs `field_size=0` | `4c795546f089` | 355 | no |
| 15 | separate declaration initializers, total then field | `4c795546f089` | 355 | no |
| 16 | initialize only under `field->type != end` guard | `e543fd41e193` | 344 | no |
| 17 | zero-valued conditional on end/non-end | `5a14c0cf7955` | 355 | no |
| 18 | arithmetic identity `total-total` | `5a14c0cf7955` | 355 | no |
| 19 | for-init-equivalent comma assignments | `5a14c0cf7955` | 355 | no |
| 20 | field zero first, total copies field | `5a14c0cf7955` | 355 | no |
| 21 | zeroed aggregate `{ total, field }` | `537a9133e3ef` | 335 | no |
| 22 | zeroed aggregate `{ field, total }` | `50c380dae9a5` | 335 | no |
| 23 | zeroed two-short array | `537a9133e3ef` | 335 | no |
| 24 | guarded do/while, initialize inside non-end guard | `5a14c0cf7955` | 355 | no |

Raw objects, copied sources, per-function disassemblies, and exact results are
under `objects/`, `sources/`, and `results.json`.  `run_matrix.py` reproduces
the whole experiment from the untouched production file.

## Independent cross-build corroboration

The locally present HCEA reconstruction at
`C:\tmp\halocea-reference\src\_data_packet_verify.c` independently records:

- its binary's initial `count` is uninitialized;
- fields outside the version range retain the previous computed size;
- the readable reconstruction deliberately initializes the value to zero.

That later build therefore carries the same original defect and its authors
made the same safety repair.  This is provenance support, not a January byte
oracle.

## Safest policy-compliant disposition

Do **not** admit `_code_0010a5f0` or `data_packets.obj` as strict matching while
the source contains the uninitialized local.  Use the readable safety fix:

```c
short field_size = 0;
short total_size;
struct data_packet_field *field;

field = fields;
total_size = 0;
```

and retain a comment such as:

```c
/* BUG (original): January leaves field_size undefined before the first field.
 * A version-ineligible first field therefore consumes allocator residue; later
 * ineligible fields reuse the previous size.  Initialize to zero in this safe
 * reconstruction. */
```

Measured result of that safe source is 464 bytes, 27 semantically identical
relocations, normalized hash `5a14c0cf...`, but it is not byte-exact and must
remain `NonMatching`.  This sacrifices one function/object credit rather than
weakening the project's no-UB rule or claiming a false exact match.

## Scope integrity

No production `.c`/`.h`, config, comparator, Git metadata, matching label, or
compiler flag was changed by this research.  All writes are confined to this
untracked research directory.
