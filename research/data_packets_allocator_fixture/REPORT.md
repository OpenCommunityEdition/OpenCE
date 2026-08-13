# `data_packets` scratch-register allocator fixture

## Scope and verdict

This is synthetic compiler research only.  No production source, config, Git
metadata, matching credit, or compiler flags were changed.

The bounded 20-variant matrix found one source-level control that reproduces
the complete January scratch-register rotation:

> In the active `data` case, declare a payload pointer and compute
> `decoded_data + sizeof(short)` **before** the validation/assert and correction
> paths.  Keep that pointer live until the later `data_encode_memory` call.

This is variant 8.  It simultaneously changes all three relevant allocator
choices to the January pattern:

| Role | Existing candidate class | January / variant 8 class |
|---|---|---|
| active-data payload pointer | ECX | EAX |
| active-raw count | EDX | ECX |
| active-array element pointer, followed by packet-definition reload | EAX, then early packet EDX | EDX, then reuse EDX for packet definition |

The important distinction from production experiment E21 is lifetime.  E21's
named pointer was short-lived near the memory call and folded away.  Variant 8
creates the pointer before the validation and both calls, so its IR lifetime
perturbs global scratch coloring even though the final payload address remains
a single `lea`.

## Toolchain and method

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Fixture: `fixture.c`, compiled independently with `/DVARIANT=0..19`
- Authoritative code hash: relocation-normalized SHA-256 from
  `tools/coff_compare.py`
- Secondary hash: SHA-256 of the emitted llvm-objdump instruction listing
  after only direct call display addresses were normalized
- Every build retained the same 736-byte function size and 37 relocations.
- Raw objects and full disassemblies are under `objects/`; the complete
  machine-readable record is `results.json`.

The fixture keeps the production walker's seven-argument signature, live
field/data/state/packet roles, active and inactive switches, validation calls,
recursive array loop, and output epilogue.  Only one source factor changes per
variant.

## Proof of the control

Baseline array setup (wrong coloring):

```asm
mov  edx,[ebp+8]       ; packet definition loaded early
lea  eax,[ebx+2]       ; element
mov  bx,[ebx]          ; count
...
mov  [ebp-10h],eax
...
push edx               ; packet definition
```

Variant 8 array setup (January coloring and lifetime order):

```asm
lea  ecx,[ebp-0Ch]
push ecx
lea  edx,[ebx+2]       ; element
mov  bx,[ebx]          ; count
lea  eax,[esi+0Ah]
push eax
mov  [ebp-10h],edx     ; preserve element
mov  edx,[ebp+8]       ; only now reload packet definition into reused EDX
push 0
push edx
call verify_fields
```

Variant 8 also emits `lea eax,[decoded_data+2]` in the data case and loads the
raw count into ECX.  Baseline emits those roles in ECX and EDX respectively.
Thus the control predicts all 27 remaining production bytes, not one isolated
instruction.

## Bounded matrix

`norm SHA-256` is the relocation-normalized function hash.  `disasm SHA-256`
is the exact recorded disassembly-listing hash described above.

| ID | One changed factor | Result class | norm SHA-256 | disasm SHA-256 |
|---:|---|---|---|---|
| 0 | Baseline: element then count; direct data pointer | baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `79501ad7c8b8062fd7618d8c518a81526868a73a35dc05c941cf32d264556af6` |
| 1 | Element assignment in verifier arg2 comma expression | extra schedule instruction | `244e037463378891e9cd66c8a22447674bb95b8e22c4cdccab469e18b6a32135` | `80df128f2386c72ef80f6a74a82eb881a664ade8d4c877343537f5aa3a1089a0` |
| 2 | Element assignment in verifier arg1 comma expression | same as V1 | `244e037463378891e9cd66c8a22447674bb95b8e22c4cdccab469e18b6a32135` | `80df128f2386c72ef80f6a74a82eb881a664ade8d4c877343537f5aa3a1089a0` |
| 3 | Element assignment in verifier arg3 comma expression | same as V1 | `244e037463378891e9cd66c8a22447674bb95b8e22c4cdccab469e18b6a32135` | `80df128f2386c72ef80f6a74a82eb881a664ade8d4c877343537f5aa3a1089a0` |
| 4 | Count before element assignment | same as V1 | `244e037463378891e9cd66c8a22447674bb95b8e22c4cdccab469e18b6a32135` | `80df128f2386c72ef80f6a74a82eb881a664ade8d4c877343537f5aa3a1089a0` |
| 5 | Element assignment after verifier | late-derivation class | `57aea20625b06dcfcb86f4cb7ab4269ec656764df923129f0322a3ab02a39c25` | `5e5ca1f5a628ee338dc3a7972eb1ff8a0f7e192c2c55bf525ebb85ca31d40fed` |
| 6 | Late packet alias | folds to baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `8084ab875a062d992611583c2c73bca6405815be9bc0831bf861c1fa6db3f165` |
| 7 | Late packet alias through comma expression | folds to baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `8084ab875a062d992611583c2c73bca6405815be9bc0831bf861c1fa6db3f165` |
| **8** | **Data payload pointer born before validation/calls** | **proven January rotation** | `aa659a0e953bcd8e5be614c35802d35f0ff971f4c433de3ff0de3ecd8f75bd9b` | `cece98ba0a10ee72044797cdf21569dfbbd2b17f89efacd8f5c1bcbe1e2df0b9` |
| 9 | Data pointer born between calls | folds to baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `8e71d11a3a01a69dc658c856ddaf046b118264d62208d0b96ba3e34214d4893c` |
| 10 | Data pointer assignment in integer-call arg | folds to baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `8e71d11a3a01a69dc658c856ddaf046b118264d62208d0b96ba3e34214d4893c` |
| 11 | Element as base then `+= 2` | folds to baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `79501ad7c8b8062fd7618d8c518a81526868a73a35dc05c941cf32d264556af6` |
| 12 | Typed `short * + 1` spelling | folds to baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `8084ab875a062d992611583c2c73bca6405815be9bc0831bf861c1fa6db3f165` |
| 13 | Element through one-field cursor aggregate | folds to baseline bytes | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `b4c06ef086bb9328c14bf79b55f092dec2da2b56747fb71672cd31934aa9da5f` |
| 14 | Preserve base; derive element after verifier | same as V5 | `57aea20625b06dcfcb86f4cb7ab4269ec656764df923129f0322a3ab02a39c25` | `9f2ece7c9b14140b8d414cc25289a9d458148ea817a8ce5ffa46e1c2925f212c` |
| 15 | Block-local raw count | folds to baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `1f1c19108ca74d636fa1a03592a80116ac03de568cd7f348c9571abc62c806b1` |
| 16 | Declaration-order restatement | folds to baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `79501ad7c8b8062fd7618d8c518a81526868a73a35dc05c941cf32d264556af6` |
| 17 | Count through `element - 2` dependency | different/worse schedule | `34421dafca214748ee88863df8d9c870d19b4440a30ef0bee428b09b8eef17a2` | `942e35f04009e35220e05e7dffd70fa7e81a01bceb8962d7ce61ef5c2526e29a` |
| 18 | Count through element, then increment | same as V1 | `244e037463378891e9cd66c8a22447674bb95b8e22c4cdccab469e18b6a32135` | `80df128f2386c72ef80f6a74a82eb881a664ade8d4c877343537f5aa3a1089a0` |
| 19 | Function-scope packet alias | folds to baseline | `b9ed4c9daefa9914828695944a7076c1f880f9ae58851c40d1d3508aaaf06576` | `a7510572a065a19698d9982dee419f69ebf62149fb64101eff2f0828aa3fd7c1` |

## Concrete production experiment

Apply only this shape inside `_data_packet_field_data`:

```c
short data_size = *(short const *)decoded_data;
byte const *data = decoded_data + sizeof(short);

match_assert(...);
if (data_size < 0 || data_size > field->count)
	data_size = 0;
data_encode_integer(state, data_size, field->count);
data_encode_memory(state, data, data_size, 1);
```

The pointer must be initialized before `match_assert`; initializing it after
`data_encode_integer` is variant 9 and compiles back to the existing candidate.
Rebuild the real TU and check all six functions with `section_infos_equal`.
The expected success signature is 768/768 bytes, 44/44 identical relocations,
and replacement of the final 27 differing normalized bytes.  If the production
context does not reproduce variant 8, stop and record that synthetic-to-real
transfer failure rather than adding casts, barriers, or undefined behavior.

## Negative findings

- Declaration order, aliases, typed-pointer spelling, a one-field aggregate,
  and a local raw count do not control the rotation.
- Comma-expression binding changes schedule length but does not produce the
  desired coloring; it should not be used in production.
- Deriving the array element after the verifier produces a distinct but wrong
  schedule.
- No flag changes, barriers, volatile qualifiers, register keywords, inline
  assembly, or undefined evaluation-order dependencies were used.
