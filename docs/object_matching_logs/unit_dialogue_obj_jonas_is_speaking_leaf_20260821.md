# `unit_dialogue.obj` Jonas is-speaking leaf

## Result

This bounded source-only wave starts from canonical commit
`a17feabb4dc694c0ea019ee65ca8f62ff7522e13`. The one requested body was
emitted in one natural candidate compile and is independently strict.
`unit_dialogue.obj` advances from 0/14 to 1/14 exact functions and from
0/4,420 to 33/4,420 meaningful code bytes. It remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_unit_is_speaking` | 33 | 48 | 1 | `cdf68311b8d581e523d6bf4b54270d7baaf30105d45213389cc8db05053340e5` |

The hardened COFF comparator proves identical padded bytes and identical
relocation address, type, destination, and addend. The sole relocation is
REL32 `_object_get_and_verify_type` at +`0x0A`. The other thirteen target
functions and all 784 target non-code bytes stay outside this wave's credit.
No previously accepted function existed in this source translation unit, so
there is no inherited exact body to disturb.

## Provenance and signature

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `unit_dialogue.obj` SHA-256:
  `72af31d8b216882215f0f1153a8f8e3796f4f176314dd155fbe315a00f679a6a`.
  Its `_unit_is_speaking` code is at PDB-confirmed RVA `0x1964A0`.
- Fresh candidate-object SHA-256:
  `dea4c7742df36cc63baf708fe57a8fecccbd26b224d4d9b3b07ec63ced830b8f`.
- Clean HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its
  `src/unit_is_speaking.c` blob is
  `31d3805736170f248a943df740a508196c27c74b`, with payload SHA-256
  `3e4840f75ad7bd7d12f123038c005f8aa13e19b976aa5030675c50ef369721c3`.
  It independently identifies the predicate as the current speech priority
  being positive.
- HCEA spells that semantic predicate with a compiler-derived integer bit
  idiom after performing its own direct datum lookup. That spelling is not
  imported. The retained repository source uses the natural typed `> 0`
  expression required by the canonical accessor and layout.
- January COFF reads the caller's `long` argument as a full 32-bit value,
  passes object mask `3` to `_object_get_and_verify_type`, compares the signed
  word at returned datum offset `0x338` against zero, emits `setg`, and returns
  a one-byte boolean. This authenticates
  `boolean unit_is_speaking(long unit_index)`.
- Canonical `units.h` supplies `unit_get(index)` through
  `_object_mask_unit`, the typed `struct unit_datum` layout, the nested
  `unit.speech.current` path, and signed `short priority`. Canonical
  `object_types.h` derives the unit mask from biped and vehicle bits, yielding
  the target value `3`; `objects.h` supplies the typed long-argument accessor
  prototype. No shared declaration or layout changes.

## Defined-C and one-shot boundary

The retained body is the direct typed operation:

```c
boolean unit_is_speaking(
	long unit_index)
{
	return unit_get(unit_index)->unit.speech.current.priority > 0;
}
```

The body was compiled once as a candidate; there were no rejected candidates,
spelling retries, declaration retries, control-flow experiments, bit tricks,
or compiler-control devices. The existing includes provide every type,
accessor, mask, and field. This translation unit defines no storage and claims
no data credit.

The source is readable defined C with one parameter per line and an explicit
return. It contains no assembly, volatile scheduling device, force-inline
annotation, optimizer pragma/barrier, raw address or byte-offset access,
pointer/integer reconstruction, pointer or union pun, inactive-union access,
undefined overflow, synthetic anchor, alignment trick, or object-byte
forcing.

## Scope and validation

Only `source/units/unit_dialogue.c` and this new Jonas-owned ledger are
changed. Protected `source/units/units.c`, vehicles, AI debug/script, actions,
their configuration and logs, all shared headers, semantic exceptions, parked
records, frozen sources, pre-existing Markdown, Claude-owned files, and data
owners are unchanged.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: `_unit_is_speaking` is strict exact at 33/48
  bytes with the required REL32 relocation.
- Semantic audit: 470 units, 4,070 functions evaluated, 3,930 semantic exact,
  3,991 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,978/11,060 exact functions,
  476,533/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Tooling suite: 179/179 tests pass; `git diff --check` is clean.
- The pre-wave fail-closed regression check reports exactly
  `_unit_is_speaking` as `NEWLY_EXACT`, `changed_nonexact: []`, and no
  inherited exact functions. Its only hard findings are the expected
  `.debug$S` and symbol-set changes caused by adding the external function
  owner.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `unit_dialogue.obj` is the final same-path reproducibility proof.
No push is performed.
