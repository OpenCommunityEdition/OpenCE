# `source/memory/hashtable.obj` strategy ledger

## Final disposition

- Campaign base: `a0bad121b7b4e37335acb3ab9a7825fb99861a7a`.
- Closing source provenance: Claude-assisted commit
  `af7288955bd217a68067f0432c72abe3f6606c5e`, transplanted as a reviewed
  single-unit delta rather than cherry-picked.
- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/memory/hashtable.obj`.
- Status: **Matching**.
- Code: **10/10 functions**, **2,400/2,400 padded bytes**, and **103/103
  relocations** strict-exact under `tools/coff_compare.py`.
- Target-owned non-code: **10/10 sections** and **257/257 logical bytes**
  strict-exact: nine `.rdata` string COMDATs (227 bytes) plus the externally
  linked 30-byte `_default_hash_polynomial` `.data` object.
- The consolidated linked-image report is **270/270 data bytes** because it
  also attributes 13 bytes of inter-COMDAT alignment/padding to this unit.
- No target-owned `.bss` section exists in this object.

`section_infos_equal` proves size, relocation count, relocation address/type,
resolved destination, and normalized SHA-256 for every code and target-owned
non-code section.

| Function | Bytes | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_hashtable_new` | 256 | 19 | `070ed08cc66e3d0932c0d107ce7df31b6453b622c617a02deecb5686b32b7879` |
| `_hashtable_set_user_data` | 16 | 0 | `d7de9ac73bd916e7fa8ee4115138af50f9dda920f19ebef3d3968c0754c1a202` |
| `_hashtable_delete` | 160 | 9 | `c3567857297826d2970e382561fcd34c8efe1b8acec4f55f4f5421a4747be2d2` |
| `_default_hash_function` | 80 | 1 | `3ebcdedf8314af82cd6e669236eb14233b87a0d46080e1fab886a4ae21d93d1d` |
| `_code_0010b270` | 288 | 4 | `2ae2a53c5c4dcbd3f00f590b438dd87ed558fa50b7a36de8c6f797c5556f9907` |
| `_hashtable_get` | 176 | 8 | `78637be8a050e4aa418a96e1ac9451d87866385eca411b7bd3959079d5702aae` |
| `_hashtable_remove` | 496 | 20 | `7fcb1c77d930a60b35fb25f9416546bf8af41830368abff166736445d9f1a114` |
| `_code_0010b630` | 160 | 7 | `ab51004e09ed335edfc3983274256411f64ed97dc1a652e6ba8fbb426f4281b9` |
| `_hashtable_grow` | 576 | 27 | `0a7974681ab4f334bda59ada858d9aed8aa9a8c9e9b9518270dd31e1f3a7e606` |
| `_hashtable_put` | 192 | 8 | `c13ac7049059a5e1ed2a498f690d1f7d2893959a36d527d32748b5a5cb0ceb61` |

## Target-owned data and ownership audit

The candidate and target define the same ten target-owned non-code sections
with identical external storage class, type, value, section size, bytes, and
relocations:

- the nine January assert/path/diagnostic strings;
- `_default_hash_polynomial`, external at offset zero of a 30-byte `.data`
  section, with the exact fifteen-word polynomial payload.

The rebuilt object additionally emits four select-any `.rdata` COMDATs:
`__real@3f800000`, `__real@00000000`, `"element_size>0"`, and `"key"`.
For each, the January split object contains an external undefined symbol with
the same name and every code relocation resolves to the same name/addend. This
is ordinary linker COMDAT pooling, not extra runtime data or an ownership
mismatch. Candidate-only `.drectve` and `.debug$S` sections are compiler/linker
metadata. The pre-edit and candidate ownership fingerprints are identical.

## Accepted source shape and house-rule audit

The closing source change is deliberately small: initialize
`old_capacity_bits` before `old_count` in `hashtable_grow`, and remove the now
stale nonmatching comment. The ordering changes VC7's IR temporary numbering
without changing the C semantics.

- Parameters remain one per line; this translation unit has no no-argument
  function requiring a vertically formatted `void` parameter.
- Every `void` definition ends with explicit `return;`; every non-void
  definition ends with an explicit value return.
- Public declarations use `struct hashtable` and the repository callback
  typedefs. There is no raw tag or object access in this subsystem, so typed
  `tag_get`/`object_get` access macros are not applicable.
- No assembly, `volatile`, force-inlining, packing/alignment pragma, undefined
  behavior, compiler-flag change, or object-byte patch is used.
- The evidence-backed early success/failure returns in `hashtable_grow` and
  `hashtable_put` are retained; both functions still end in an explicit return.

## Preserved experiment history

The donor branch measured the final `_hashtable_grow` residual as a two-byte
EAX/ECX permutation at the line-136 commutative assertion
`table->capacity_bits+growth_bits<SHORT_BITS`. Operand swapping, widening casts,
and polarity respelling were inert because VC7 canonicalized the expression.
The preserved bounded initializer-order matrix was:

| Source shape | Differing normalized bytes | Decision |
|---|---:|---|
| `old_count` before `old_capacity_bits` | 2 | replaced |
| `old_capacity_bits` before `old_count` | 0 | accepted, strict exact |
| slots first | 30 | rejected |
| capacity last | 244 | rejected |
| elements first | 255 | rejected |
| assertion evaluated through `old_capacity_bits` | 172 | rejected |

The accepted ordering gives the capacity-word reload ECX affinity and extends
`growth_bits` through EAX, exactly matching January. Do not repeat blind
operand reordering or introduce prohibited compiler controls.

## Fresh campaign-base admission evidence (2026-08-20)

- A clean pre-edit regression snapshot was captured at exact base
  `a0bad121b7b4e37335acb3ab9a7825fb99861a7a`.
- The candidate object was deleted and force-rebuilt with XDK 3911.
- The hardened function sweep passed 10/10 with 2,400/2,400 padded bytes and
  103/103 relocations.
- The hardened non-code/ownership sweep passed 10/10 target-owned sections,
  257/257 logical bytes, and zero target-owned BSS sections.
- The fail-closed pre-admission regression check retained all nine previously
  exact functions, reported only `_hashtable_grow` as newly exact, and found
  zero changed nonexact siblings, data changes, or symbol-ownership changes.
- Clean full `halobetacache_build` and `libcmt_build` targets passed after 570
  compilation edges.
- Consolidated semantic progress scanned 470 units and 3,604 functions,
  accepted 3,521 exact functions, and reported zero unit errors.
- Consolidated progress reports this unit at 10/10 functions,
  2,343/2,343 meaningful code bytes, and 270/270 linked data bytes. Campaign
  Halo progress is 268/468 complete objects.
- The admission audit reports zero candidates and zero revocations involving
  this unit. Its sole contradiction is the pre-existing unrelated
  `source/shell/shell_xbox` record.
- `python -m unittest discover -s tools -p "test_*.py" -v`: 179/179 passed.
- `git diff --check` and the four-file scope/frozen-object policy audit passed.

No fuzzy percentage, semantic exception, parked credit, compiler override, or
ownership waiver is used for this admission.
