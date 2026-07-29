# `prime_numbers.obj` - January 2002 / XDK 3911

Target: `build/split/source/bungie_net/common/prime_numbers.obj`.
Candidate: `build/base/source/bungie_net/common/prime_numbers.obj`.

## Inventory and current strict state

| Symbol/data | Target | Candidate | Strict state |
| --- | --- | --- | --- |
| `_compare_prime_numbers_descending` | 32 bytes, 0 relocations, `2e54965488b9bb491fcdf3697701f87ce86c0ea930186ed7a9c765b3fcf5bdc9` | identical | exact |
| `_generate_prime_numbers` | 352 bytes, 12 relocations, `2428a7a00be6ab6f96efe9dda273d824a0cc9829aeeb2dd5f4c9b35b2b7992b1` | 352 bytes, 12 relocations, `66e1c7859e354a6bf7caccb70b260e99d516be6a94837571ecc60dd5b43edda6` | nonmatching |
| `_randomprime` | 80 bytes, 4 relocations, `4741be658ca28e451309f235d46c6f2f8760301ccb232c856d2ee1a46132c2bd` | identical | exact |
| `_probable_prime64` | 192 bytes, 10 relocations, `ece78fdfc3ae49468f8a5634141148861548ab6b7b87faf38ad5d5a26a32b144` | identical | exact |
| target `.rdata` payloads | 8, 11, and 49 bytes; no relocations | matching 8, 11, and 49 byte payloads | exact payloads |
| candidate-only `.drectve` / `.debug$S` and 7-byte `result` assertion literal | not emitted by csplit target object | emitted by compiler | non-code TU metadata/data difference; not strict-exact as a full object |

The source and target relocation destinations agree for all 12 `generate_prime_numbers` relocations. Their addresses do not: target has `38,43,48,55,87,106,142,149,277,293,312,322`; candidate has `38,43,48,55,91,111,140,147,275,291,310,320`.

## Baseline and source-shape experiments

| Shape | Result | Decision |
| --- | --- | --- |
| Reconstructed baseline from `b6883168a` / `a82502346` | `generate_prime_numbers`: 336 bytes, 12 relocs, hash `ba031c73d69fc3cc6bbfbe9046a46c18c7b1d3662f15971d6df4ff746ef3860b`; first raw divergence offset 5 (`sub esp, 8` vs target `sub esp, 12`). | superseded |
| Move `k = 3` before the `sqrt` conversion (uncommitted inherited experiment) | 336 bytes, 12 relocs, hash `bea188af3051b10618c03d9fe606334d415cc03625617811931637be33b3e987`; first divergence offset 10 and register allocation changes (`EBX`/`ESI` no longer match target). | reverted |
| Earlier `prime-numbers-final` worktree shape (`primes = num_primes`, do/while rewrites, `sieve_count`, allocation rewrite) | 368 bytes, hash `82e7e071c6b74de906e2e60ec782db70447530a884adc62e3b63879f7ee606a5`; first divergence offset 10, with different maximum/`num_primes` register ownership. | rejected |
| Current focused shape: allocation as `odd_count * sizeof(*primes) + sizeof(*primes)`, failure folded into `if (primes)`, and an explicit `sieve_count` lifetime | 352 bytes and 12 relocs (both target values); hash and relocation addresses still differ. Objdiff: 81.61%. | accepted, not exact |
| Write `*num_primes = i` rather than literal zero in the `< 2` branch | compiler folded it to the same current object hash; no strict effect. | reverted; do not repeat |

## First divergence and blocker

The current candidate's first raw difference is offset 5: target `83 EC 0C` (`sub esp, 0xC`) versus candidate `83 EC 08` (`sub esp, 0x8`). The target keeps three stack slots (`total_count` at `-0xC`, then live ranges at `-0x8` and `-0x4`); the candidate coalesces the corresponding lifetimes into two. This then changes source-independent scheduling around malloc, x87 conversion, and the sieve, shifting relocations although their ordered destinations and types remain identical.

No inline assembly, `volatile`, undefined behavior, byte editing, or compiler-flag changes were used.

## Do not repeat

- Do not move `k = 3` before the `sqrt` conversion: it breaks the otherwise matching initial register plan.
- Do not use the `primes = num_primes`/do-while reconstruction from `prime-numbers-final`; it creates a distinct allocation plan and a 368-byte function.
- Do not substitute `i` for the zero literal in the early result path; XDK3911 optimizes it identically.

## 2026-07-29 full-admission continuation

Fresh baseline at canonical `5b61655c5` reproduced the inventory above with
XDK 3911 CL `13.00.9254.1` and `/O2 /Oy- /DDEBUG /Dxbox`. The target-owned
8-, 11-, and 49-byte `.rdata` payloads are exact. Three functions are strict
exact; `_generate_prime_numbers` remains `352/352` bytes and `12/12`
relocations.

### E17 - reuse the four loop variables by their target lifetimes

- Source shape: replaced the separate `scan_count` and `sieve_count` locals
  with the existing `j` and `k` lifetimes inferred from the January
  disassembly: `j` scans to the square-root bound, `k` changes from the odd
  fill value to the outer sieve countdown, `i` changes from the fill index to
  the divisor index, and `j` becomes the inner sieve index. `k = 3` was placed
  immediately before the square-root conversion.
- Result: `352/352` bytes, `12/12` relocations, candidate SHA
  `f857c1d5c1506df254332a262b25d998428b592dece555572e6452f45175df6c`.
  The lifetime reuse is semantically and structurally plausible, but the
  block-local `k` start assigns `odd_count/num_primes` to `ESI/EBX` instead
  of the target `EBX/ESI`; the first relocation moves from target `+0x26` to
  candidate `+0x2c`. All three exact siblings remain exact.
- Disposition: the lifetime reuse is retained for the next placement test,
  but the block-local initialization is rejected.
- Do not repeat: this reuse pattern with `k` first becoming live only inside
  the successful-allocation block.

### E18 - reuse lifetimes with entry-live `k`

- Source shape: retained E17's four-variable lifetime reuse but moved
  `k = 3` beside the entry `i = 0; j = 0` initializers. This directly tested
  whether the reuse pattern fixed the stack homes once the previously proven
  entry-live frame/register control was applied.
- Result: `352/352` bytes, `12/12` relocations, candidate SHA
  `87a46a9899f790be03808570c3d442cfc7e992e8f6cd2730de0047d52aad377d`.
  It preserves the target `EBX=odd_count`, `ESI=num_primes`, `EDI=i` plan and
  12-byte frame, but is byte-identical to historical E12: `k` is homed at
  `-4`, the scan index at `-8`, and the `k = 3` store is emitted before the
  assertion rather than at target `+0x83`. The lifetime reuse itself does not
  change allocation. Exact siblings remain exact.
- Disposition: rejected as a duplicate of the E12 allocation class.
- Do not repeat: merely replacing the named scan/countdown locals while
  leaving initializer order `i, j, k`.

### E19 - reverse the entry `k` and scan-index assignments

- Source shape: retained E18's lifetime reuse and entry-live `k`, but assigned
  `k = 3` before `j = 0` to test whether first-store order controls the two
  colored stack homes.
- Result: `352/352` bytes, `12/12` relocations, candidate SHA
  `8b5a95d2331c54ddb0dd454984eaf9928b3c908d20b3a0d3c596f442c2f5247b`.
  The target register plan and 12-byte frame remain correct, but the homes do
  not exchange: fill/countdown `k` remains at `-4`, scan `j` remains at `-8`.
  Only the entry store order changes. Exact siblings remain exact.
- Disposition: rejected; assignment order is not the stack-coloring control.
- Do not repeat: entry assignment permutations alone.

### E20 - use the live zero index for the two target zero operands

- Source shape: on E19, changed the early `*num_primes = 0` to `i` and called
  `debug_malloc` directly with `(boolean)i` instead of the macro's literal
  `FALSE`, targeting January's `mov [esi],edi` and `push edi`.
- Result: byte-identical to E19: `352/352`, `12/12`, SHA
  `8b5a95d2331c54ddb0dd454984eaf9928b3c908d20b3a0d3c596f442c2f5247b`.
  VC7 proves `i` is zero and rematerializes immediates; exact siblings remain
  exact.
- Disposition: rejected and restored to house macros/literals.
- Do not repeat: source aliases for constant zero, alone or together.

### E21 - pair fill/countdown and scan state in an aggregate

- Source shape: replaced the two scalar lifetime classes with an eight-byte
  `prime_generator_state` whose first field is the fill value/countdown and
  whose second field is the scan index. This tested whether aggregate layout
  could force January's adjacent `k=-8`, `scan=-4` homes without unsafe
  aliasing.
- Result: `352/352` bytes and `12/12` relocations, candidate SHA
  `edebb85b569deeca14b12fd22ff09f8d59e5d27059b3d5fc98778145a8dba82c`.
  VC7 partially scalarizes the aggregate: the fill value occupies `EBX`,
  `odd_count` spills to `-4`, scan occupies `-0xc`, and the frame grows to
  `0x10`. Relocation destinations/order remain correct but addresses drift.
  Exact siblings remain exact.
- Disposition: aggregate retained only for the next entry-live-field test;
  block-live aggregate initialization is rejected.
- Do not repeat: a block-live two-field state aggregate.

### E22 - make both aggregate fields live at entry

- Source shape: retained E21's two-field state but initialized the fill value
  to 3 beside the entry scan-index zero, before the assertion.
- Result: this is the strongest new allocation control. It produces the
  target 12-byte frame, `EBX=odd_count`, `ESI=num_primes`, `EDI=i`, and the
  exact physical homes `fill/countdown=-8`, `scan=-4`. Candidate SHA is
  `c5d8083d66f4eaa4abe8dd0d38c42bf468a67fcfeda5d6a8b44682c62585f65e`
  at `352/352` and `12/12`. The sole early structural problem is that the
  `fill=3` store appears at `+0x1c`, while January emits it at `+0x83` after
  successful allocation. Downstream loop homes now match. Exact siblings
  remain exact.
- Disposition: retain the aggregate layout as the best evidence and test
  whether an address-based field lifetime can preserve it without the early
  store.
- Do not repeat: plain entry initialization; the store timing is wrong.

### E23 - make the fill/countdown field address-live before its late store

- Source shape: retained E22's two-field aggregate, took the address of its
  fill/countdown field before the assertion, and performed every later access
  through that pointer. The actual `3` store remained after successful
  allocation. This tested whether field-address lifetime could preserve
  E22's stack layout without emitting January's store too early.
- Result: `352/352` bytes, `12/12` relocations, candidate SHA
  `edebb85b569deeca14b12fd22ff09f8d59e5d27059b3d5fc98778145a8dba82c`.
  VC7 proves the pointer alias and removes it; output is byte-identical to E21.
  The frame grows to `0x10`, fill scalarizes into `EBX`, `odd_count` spills,
  and relocation addresses begin at `+0x2c` instead of target `+0x26`.
  Exact siblings remain exact.
- Disposition: rejected.
- Do not repeat: an address alias to only the fill/countdown field; it does
  not keep the containing aggregate allocated.

### E24 - model all three target homes as one aggregate

- Source shape: expanded the aggregate to three fields in target stack order:
  total count, fill/countdown, and scan index. The total-count field becomes
  live before allocation, the scan field at entry, and the fill field only
  after allocation. All later uses referenced their aggregate fields.
- Result: `352/352` bytes, `12/12` relocations, candidate SHA
  `99cef5d183f828bf53c169fdfe1566b0364b0b8ffc315c57bfe26be27eb9f2a9`.
  VC7 again performs partial scalar replacement rather than preserving the
  source layout: the frame is `0x10`, `odd_count` spills at `-4`,
  `scan_index` occupies `-8`, total count occupies `-0x10`, and fill remains
  in `EBX`. The first relocation is at `+0x2c`, not target `+0x26`.
  Exact siblings remain exact.
- Disposition: rejected.
- Do not repeat: a directly accessed three-field struct; source member order
  does not constrain the optimized physical homes.

### E25 - model the three target homes as a constant-index array

- Source shape: replaced E24's struct with a three-element local array and
  named indices for total count, fill/countdown, and scan index. This tested
  whether array semantics prevented the member-wise scalar replacement seen
  with the struct.
- Result: byte-identical to E24: `352/352`, `12/12`, SHA
  `99cef5d183f828bf53c169fdfe1566b0364b0b8ffc315c57bfe26be27eb9f2a9`.
  VC7 scalarizes constant-index array accesses into the same frame and
  register allocation. Exact siblings remain exact.
- Disposition: rejected.
- Do not repeat: a constant-index local array without an escaping address.

### E26 - access the complete aggregate through a local pointer

- Source shape: restored E24's three-field struct, took its address at entry,
  and performed every member access through the whole-struct pointer. Unlike
  E23, the alias covered every candidate home.
- Result: byte-identical to E24/E25: `352/352`, `12/12`, SHA
  `99cef5d183f828bf53c169fdfe1566b0364b0b8ffc315c57bfe26be27eb9f2a9`.
  VC7 proves away the local pointer and applies the same partial scalar
  replacement. Exact siblings remain exact.
- Disposition: rejected.
- Do not repeat: local aliases to the complete aggregate; no address escapes,
  so they do not constrain allocation.

### E27 - use a signed fill/countdown lifetime

- Source shape: restored the closest four-variable lifetime reuse and changed
  only the fill/countdown variable from `unsigned long` to `long`. January's
  machine code only stores, increments, decrements, and zero-tests this value,
  so signedness is semantically neutral for the valid range while presenting
  a genuinely different optimizer type.
- Result: `352/352` bytes, `12/12` relocations, candidate SHA
  `f857c1d5c1506df254332a262b25d998428b592dece555572e6452f45175df6c`,
  byte-identical to E17/E2/E5/E7. The fill/countdown remains register-held,
  and entry ownership remains `ESI=odd_count`, `EBX=num_primes`, contrary to
  January's `EBX/ESI` assignment. Exact siblings remain exact.
- Disposition: rejected; type signedness is not the missing control.
- Do not repeat: integer signedness changes for this lifetime.

## Final disposition

Evidence exhaustion was reached after 27 documented legal-C source families
plus the inherited count-model and loop-shape work. The retained production
source is the semantics-correct baseline: it preserves the in-bounds
`odd_count + 1` allocation/count model, target function size, relocation count,
and exact relocation destination order. It does not claim strict equality.

The only nonexact function remains `_generate_prime_numbers` at `352/352`
bytes and `12/12` relocations. The target simultaneously requires a 12-byte
frame, `EBX=odd_count`, `ESI=num_primes`, `EDI=i`, a late-initialized
fill/countdown value in `[ebp-8]`, scan state in `[ebp-4]`, and total count in
`[ebp-0xc]`. Every measured legal-C control selects only a subset: late
initialization changes the entry register plan or permits scalar replacement;
entry initialization fixes allocation but emits a store too early; aggregates,
arrays, and local aliases are scalar-replaced; declaration, spelling, hint,
and signedness changes are neutral.

Classification: evidence-backed `tu-context-optimization` blocker. Reopen only
with original-source/local-variable provenance, a verified compiler-patch
difference, or a legal-C donor that demonstrates the complete allocation and
late-store combination under XDK 3911 CL 13.00.9254.1 and the campaign flags.

House-style audit: parameters remain one-per-line; allocation/free/reallocation
use the house match macros; no raw tag/object access is involved; no alignment
directive, assembly, `volatile`, undefined behavior, byte patch, or compiler
flag change is present. Existing early returns and the exact comparator's
branching are retained where restructuring would change proven machine code.

## Validation and reopen criteria

Final validation on 2026-07-29:

- XDK 3911 CL `13.00.9254.1`, `/O2 /Oy- /DDEBUG /Dxbox`.
- Full `ninja halobetacache_build`: success, all 466 object actions complete.
- `ninja progress`: success. `prime_numbers.obj` reports 3/4 functions
  (`75%`) and 291/631 meaningful code bytes credited (`46.12%`); ordinary
  fuzzy text similarity is `90.09%`, and `_generate_prime_numbers` alone is
  `81.61%`. These ordinary percentages are not strict-exact credit.
- Hardened function census: comparator, random-prime wrapper, and 64-bit
  probable-prime generator exact; `_generate_prime_numbers` nonexact at
  `352/352` padded bytes and `12/12` relocations.
- Target-owned `.rdata`: all three payloads exact, with sizes/hashes
  `8/b0146a2c...`, `11/41656026...`, and `49/56c8d208...`; objdiff reports
  72/72 aligned data bytes (`100%`). Ownership names and storage classes
  agree. Candidate-only compiler/debug sections and the select-any `result`
  assertion literal are documented csplit/compiler artifacts, not target
  data ownership.
- Tool tests:
  `python -m unittest -v tools.test_regression_gate
  tools.test_audit_semantic_matches tools.test_semantic_progress`:
  40/40 pass.
- `config/config.json` remains `NonMatching`; no exact/Matching credit was
  added and no existing credit was weakened.

Reopen only with source evidence that preserves the target’s `EBX=odd_count`, `ESI=num_primes`, and `EDI=i` entry plan while forcing the target’s three distinct stack-lifetime slots. Required acceptance is strict equality of size, normalized hash, and relocation address/type/destination for all four functions, plus the TU’s non-code data/metadata comparison and the campaign regression manifest.
