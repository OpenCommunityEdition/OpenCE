# `source/memory/hashtable.obj` strategy ledger

## Final disposition

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
non-code section. The complete per-function evidence is recorded below.

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

The candidate and target define the same ten non-code owners with identical
external storage class, type, value, section size, bytes, and relocations:

- the nine January assert/path/diagnostic strings;
- `_default_hash_polynomial`, external at offset zero of a 30-byte `.data`
  section, with the exact fifteen-word polynomial payload.

The rebuilt object additionally owns four select-any `.rdata` COMDATs:
`__real@3f800000`, `__real@00000000`, `"element_size>0"`, and `"key"`.
For every one, the January split object contains an external undefined symbol
with the same name, and all code relocations resolve to the same name/addend.
This is ordinary linker COMDAT pooling: csplit attributes the selected copy to
another object, while MSVC emits a discardable local candidate copy here. It
does not represent extra runtime data or an ownership mismatch. Candidate-only
`.drectve` and `.debug$S` sections are compiler/linker metadata and are likewise
not game data.

## Recovered source and ABI

- All public declarations use the repository's named `struct hashtable` and
  callback typedefs; no raw-layout casts are used.
- Target and candidate code sections are byte-identical, so stack cleanup,
  cdecl argument order, callee-saved register behavior, return registers, and
  all private helper conventions are identical by construction. Every target
  function ends in plain `ret`, consistent with the C declarations.
- Parameters follow the house layout (one per line), and every `void` function
  ends in explicit `return;`. There are no no-argument functions in this TU.
- No assembly, `volatile`, `__forceinline`, packing/alignment pragma, undefined
  behavior used as a codegen lever, compiler-flag change, or byte patch is
  present.
- `hashtable_grow` and `hashtable_put` retain their evidence-backed early
  success/failure returns. The house rule explicitly treats single-exit as a
  strong tendency rather than a law; reshaping these exact control flows would
  be needless codegen contortion.

## Preserved experiment history

The last `_hashtable_grow` residual was a two-byte EAX/ECX permutation while
evaluating the line-136 commutative assertion
`table->capacity_bits+growth_bits<SHORT_BITS`. Operand swapping, widening casts,
and polarity respelling were inert because VC7 canonicalized the expression.
Snapshot initializer order was the real IR input:

| Shape | Differing normalized bytes | Result |
|---|---:|---|
| `old_capacity_bits` initialized before `old_count` | 0 | accepted, strict exact |
| slots first | 30 | rejected |
| capacity last | 244 | rejected |
| elements first | 255 | rejected |
| evaluate through `old_capacity_bits` in the assertion | 172 | rejected |

The accepted ordering gives the capacity-word reload ECX affinity and extends
`growth_bits` through EAX, exactly matching January without changing semantics.
Do not repeat blind operand reordering or introduce prohibited compiler levers.

## Admission gates

- targeted XDK rebuild: passed;
- 10/10 function strict COFF gate: passed;
- 10/10 target-owned non-code/ownership gate: passed;
- full `ninja halobetacache_build`: passed (466 translation units rebuilt,
  exit 0);
- `ninja progress`: passed; the raw report gives this unit 10/10 functions,
  2,343/2,343 meaningful code bytes, 270/270 linked data bytes, and one
  complete unit, while the hardened report independently lists all ten
  functions as semantic-exact with no ordinary-only exception;
- `python -m unittest discover -s tools -p "test*.py"`: 173/173 passed;
- `git diff --check`: passed.
