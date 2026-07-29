# circular_queue.obj exact-match strategy ledger

## Scope and required toolchain

- Translation unit: `source/memory/circular_queue.c`
- Target: January 2002 `source/memory/circular_queue.obj`
- Compiler: XDK 3911 CL 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Strict gate: `tools/coff_compare.py` / `section_infos_equal`
- Detailed experiment transcript:
  `research/circular_queue_obj/EXPERIMENTS.md`

No inline assembly, volatile byte forcing, undefined behavior, object-byte
patching, compiler-flag changes, or fuzzy-match credit was used.

## Validated baseline

Seven of eight functions are strict exact:

| Function | Target/base size | Relocs | Normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `circular_queue_reset` | 16/16 | 0/0 | `43bd388fa461cee2...` | exact |
| `circular_queue_validate` | 112/112 | 6/6 | `f6a98f7f32feb7df...` | exact |
| `circular_queue_new` | 96/96 | 4/4 | `89dac4b56b758d67...` | exact |
| `circular_queue_delete` | 48/48 | 3/3 | `90fd5b668d0661b3...` | exact |
| `circular_queue_size` | 32/32 | 1/1 | `691530e94ba7f3f2...` | exact |
| `circular_queue_free_space` | 48/48 | 1/1 | `fa1cb9aa5c44d00d...` | exact |
| `circular_queue_queue_data` | 240/240 | 12/12 | `cfe1f5adce7a13f3...` | exact |
| `circular_queue_dequeue_data` | 256/240 | 12/12 | target `2981341a22238a314c0f885d555149dc279257b6109242e674f38ccfb0ccd1f9`; base `40adf3be1fa79c0b9aaf6b169e37da2e59aa4e306efef68a9573b7aa7ec8dede` | nonexact |

The strict code result is therefore 7/8 functions and 592/848 target padded
code bytes. All five owned `.rdata` COMDATs are raw-byte exact:
46 + 39 + 65 + 52 + 49 = 251/251 bytes. The object owns no `.data` or
`.bss`.

The residual begins at normalized offset `+0x4`; 222 normalized bytes differ.
The first relocation has the correct `_circular_queue_validate` destination
but is at target/base address 14/12. January keeps the queue in `EDI`, the
input size in `ESI`, initializes `BL` to false, later reuses `EBX` for
`read_offset`, and returns `BL` only on failure. Current VC7 instead keeps the
queue in `EBX`, `read_offset` in `EDI`, and folds false directly into `AL`.

## January and cross-build provenance

- The October 2001 XBE function at VA `0x00118fb0` has the same machine
  topology instruction-for-instruction, including `xor bl,bl`, the
  `EBX=read_offset` lifetime reuse, and separate success/failure tails.
- January's PDB exposes the public symbol but no local-variable or line
  records for this function.
- HCEA exposes only the later `simple_circular_queue_*` family and is not a
  source donor for this implementation.
- A campaign-wide target scan found no second strict donor combining early
  byte-zero lifetime materialization with reuse of that register as a later
  long local and a final same-byte failure return.
- The two locally available authentic VC7 compilers, 13.00.9254.1 and
  13.00.9210, emit the same baseline hash from unchanged source.

## Accepted production source

The production source remains the safe semantic reconstruction with one
result local, one success block, an optional `read_offset` writeback, and a
single final return. It preserves all seven exact sibling functions and every
owned data section. No experimental source shape was retained because none
strictly improved the complete translation unit.

## Experiment matrix

Every row was built with the required XDK compiler and flags. Unless stated
otherwise, all seven exact siblings and all five `.rdata` sections remained
exact. Hashes ending in `...` are historical prefixes: the earlier session
did not preserve the full digest, and the experiment was not rerun merely to
lengthen a digest.

| ID | Exact source delta | Base size/relocs/hash | First divergence | Sibling/data result | Decision |
| --- | --- | --- | --- | --- | --- |
| N01 | Early failure return of initialized result; literal `TRUE` success return. | `0xF0`/12, nonbaseline | `+0x4`; queue still `EBX`, fail `xor al,al` | exact | reverted |
| N02 | Literal `TRUE` return inside canonical success block; result only for trailing fail. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | no effect; reverted |
| N03 | Add redundant `!result` to the success guard. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | predicate eliminated; reverted |
| N04 | Union-overlay `boolean result` and `long read_offset`. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | scalarized; reverted |
| N05 | Put initialized result first in declaration order. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | no effect; reverted |
| N06 | `_ReadWriteBarrier()` immediately after false initialization. | `0xF0`/12, `f803ad9d...` | historical trace `+0x6` (`+0x7` under the later normalized reporter) | exact | prologue partly moves, false still folds; reverted |
| N07 | N06 plus historical `if (result && advance)` writeback topology. | `0xF0`/12, `9ae0aa56...` | `+0x6` | exact | wrong shared-tail topology; reverted |
| N08 | Late `switch(result)` with distinct success/default returns. | `0x100`/12, `ac2dc1a8...` | `+0x3` | exact | spills result and emits switch machinery; reverted |
| N09 | Replace N08 switch with `if (result == TRUE)`. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | branch folds; reverted |
| N10 | Switch directly on the runtime availability predicate. | `0x100`/12, `107cae55...` | `+0x4` | exact | `setle`/switch code is not January; reverted |
| N11 | N06 barrier plus N09 explicit outer success branch. | `0xF0`/12, `f803ad9d...` | `+0x6` | exact | no improvement over N06; reverted |
| N12 | Barriers after false initialization and after success assignment. | `0xF0`/12, `1f080135...` | queue remains in `EBX` | exact | no BL lifetime; reverted |
| N13 | Scope `read_offset` and `contiguous_size` inside success block. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | no effect; reverted |
| N14 | N13 plus `register boolean result`. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | hint ignored; reverted |
| N15 | N14 plus full-function `queue_reference` alias. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | alias eliminated; reverted |
| N16 | Use `result++` on success. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | folded to canonical; reverted |
| N17 | Store result in a one-byte local struct. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | aggregate scalarized; reverted |
| N18 | Single-iteration `do` loop, failure `break`, success assignment. | `0x150`/17, `8e348c3ab923622446e3d5b4ff940cfd3e6466990d4e4aa793f8343d121e3f93` | `+0x5`; 267 bytes | exact | **creates target BL/EBX lifetime**, but clones validator into the size path (+80 bytes/+5 relocs); reverted |
| N19 | N18 plus noinline validator. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | removes both clone and desired lifetime; reverted |
| N20 | N18 plus manually expanded size expression. | `0x150`/17, `8e348c3a...` | `+0x5` | exact | same nested validator inline; reverted |
| N21 | N20 validator call through a local function pointer. | `0x100`/12, `9339ad1b...` | `+0x4`; 68 bytes | validator and exact caller cluster regress; data exact | closest dequeue body, but address-taking destroys target private EAX ABI; reverted |
| N22 | N20 plus noinline validator. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | returns to baseline; reverted |
| N23 | Single-iteration `do ... while (result == FALSE)`. | `0x150`/17, `8e348c3a...` | `+0x5` | exact | same nested-inline fixed point; reverted |
| N24 | N23 plus noinline validator. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | returns to baseline; reverted |
| N25 | N23 plus barrier at validator body entry. | `0x150`/17, `8e348c3a...` | `+0x5` | exact | barrier cannot prevent cloned inline; reverted |
| N26 | N23, noinline validator, barrier after false initialization. | `0xF0`/12, `f803ad9d...` | `+0x7`; 218 bytes | exact | N06 fixed point; reverted |
| N27 | `while(TRUE)` with explicit failure and success breaks. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | simplified to canonical; reverted |
| N28 | Explicit `goto dequeue_done` failure edge. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | simplified to canonical; reverted |
| N29 | Force-inline helper returning false for initialization. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | helper folded; reverted |
| N30 | Force-inline helper returning availability predicate. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | helper folded; reverted |
| N31 | Declaration initializer plus inner advance and late success assignment. | `0xF0`/12, `40adf3be...` | `+0x4` | exact | no effect; reverted |
| N32 | Reverse comparison spelling to `size >= data_size`. | `0xF0`/12, `bbe01e24...` | `+0x4` | exact | wrong compare orientation, allocation unchanged; reverted |
| N33 | Assign result before optional advance. | `0xF0`/12, `c3d7f616...` | historical trace retained only as success-tail divergence | exact | creates wrong shared success epilogue; reverted |
| N34 | Force-inline validator wrapper plus N18. | `0x150`/17, `8e348c3a...` | `+0x5` | exact | wrapper recursively inlines; reverted |
| N35 | Ordinary static validator wrapper plus N18. | `0x150`/17, `8e348c3a...` | `+0x5` | exact | wrapper inlines; reverted |
| N36 | Move validator definition after reset, matching January symbol order. | `0xF0`/12, `40adf3be1fa79c0b9aaf6b169e37da2e59aa4e306efef68a9573b7aa7ec8dede` | `+0x4`; 222 bytes | exact | definition order is not the missing control; reverted |
| N37 | N36 definition order plus N18 loop. | `0x150`/17, `8e348c3ab923622446e3d5b4ff940cfd3e6466990d4e4aa793f8343d121e3f93` | `+0x5`; 267 bytes | exact | exactly reproduces N18; reverted |

## Residual classification

Measured fact: target BL/EBX lifetime is reachable with this compiler (N18),
but every legal-C shape found that creates it also changes the inliner fixed
point and clones the validator, yielding 17 rather than 12 relocations.
Preventing the clone—by noinline, a function boundary, or canonicalized
control flow—simultaneously destroys the lifetime. Address-taking produces a
much closer 256/12 dequeue but globally changes the validator's proven private
ABI and regresses exact siblings.

The best classification is **TU-context optimizer/inliner-allocation fixed
point**, not a proven compiler-QFE difference and not a semantic gap. Because
the current residual has unequal padded sizes, it is research-parked here but
is deliberately **not** admitted to `config/parked.json` under the stricter
equal-size/equal-relocation parking rule.

## Do-not-repeat list

- Do not retry declaration order/scope, `register`, local aliases, scalar
  unions/structs, result increment, reversed comparisons, or result-write
  placement.
- Do not retry single-iteration loops, `while(TRUE)`, `goto`, early returns,
  shared tails, switches, or redundant predicates.
- Do not retry barriers at result or validator sites, validator
  noinline/forceinline/static wrappers, or manual size expansion.
- Do not take the validator's address: it invalidates the exact private ABI
  used by the rest of the translation unit.
- Do not retry function-definition order; both canonical and BL-producing
  control-flow shapes were measured.
- Do not use inline assembly, volatile byte forcing, undefined behavior,
  object-byte patches, altered optimization flags, or fuzzy credit.

## Reopen criteria

Reopen only if one of these appears:

1. January source, line/scope records, or a genuine same-lineage C donor
   reveals a new control topology.
2. A strict-exact donor demonstrates BL/long-register lifetime reuse without
   changing an exact private helper ABI.
3. A distinct authentic Bungie-era VC7 13.00.9254 compiler binary improves
   unchanged source.
4. A tested general compiler control can prevent nested cloning without
   forcing noinline or address-taking.

## Disposition

The safe source is restored and remains 7/8 strict exact with 251/251 owned
data bytes exact. `circular_queue.obj` is **not Matching** and receives no
object credit. The residual is evidence-exhausted under current provenance and
parked at the research level pending one of the explicit reopen criteria.
