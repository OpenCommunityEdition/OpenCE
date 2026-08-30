# `object_lights.obj`: light attenuation recovery (2026-08-29)

## Result

`_light_attenuation`, formerly `_code_00129040`, is strict-exact on the first
and only production candidate compile. The wave adds one function and 32
padded code bytes. `source/objects/object_lights.obj` advances from 11/43 to
12/43 strict functions and from 656/12,608 to 688/12,608 strict padded bytes.
The campaign board advances from 4,725/8,246 to 4,726/8,246 strict functions
and from 654,103/1,922,669 to 654,135/1,922,669 strict padded bytes. The object
remains `NonMatching`; no completion, semantic exception, park, or data-credit
claim is made.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_light_attenuation` | 25 / 32 | 1 | `3ccf5232cc3c8f563f10d54ed584335f7ae9fc4b2f7c29819e44408657d017f0` |

Every meaningful and padded byte agrees. The sole zero-addend `DIR32`
relocation is at `+0x13` to `__real@3f800000`; its address, type, destination,
owner, and addend agree with January. Target and candidate symbols are
external storage class 2 functions of type `0x20`.

## January and HCEA evidence

The January target COFF and disassembly are the code and ABI authority. They
prove an ordinary external cdecl function returning a `real` in `ST(0)` from
two stack `real` parameters. January squares the second parameter, squares the
first, divides the former by the latter, and reverse-subtracts the quotient
from the relocated `1.0f` constant. The resulting expression is uniquely:

```c
1.0f - (distance * distance) / (radius * radius)
```

The later HCEA symbol corpus at commit
`aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`, blob
`5944b14b92b42902322c4b3f858665f9d25b2067`, independently names this body
`light_attenuation(real radius, real distance)` and supplies the same formula.
HCEA's README records that the corpus was recovered from the HCEA prototype's
associated verbose symbol data. This is later-build naming and semantic
evidence, not an independent January symbol record. The January body,
parameter order, return ABI, translation-unit placement, and exact historical
anonymous build corroborate its application here.

The natural ownership update renames the code entry at executable file offset
1,216,576 in `config/symbols.json` from `_code_00129040` to
`_light_attenuation`. The flags remain 32. No entry is added to
`config/symbol_ownership.json`: that file describes reviewed non-code section
ownership, while this is an externally owned function COMDAT. A scan of all
833 January split objects found no undefined reference to the old anonymous
name, and the target Object Lights object has no relocation caller; the
compiler retained the ordinary external body independently of its inlining.

## Prior-attempt census

A read-only census covered 1,497 registered worktrees, 1,461 refs, all six
unique committed `object_lights.c` blobs, and all eight unique historical
Object Lights ledger blobs. No committed source, ledger, or configuration
change had implemented, named, compiled, rejected, or admitted this target.
The anonymous name had remained unchanged since the initial symbol import.

One uncommitted July artifact survives at detached dirty worktree
`C:/tmp/object-lights-lane`, HEAD
`a32e704befe11589e664c4a69a90facfde3da035`. Its source blob
`b7869517634321f0cf6af8fb2380e5248f8cfed9` contains the anonymous body
`1.f - (value1*value1)/(value0*value0)`. Its preserved object is strict-exact
with the same 25/32-byte and one-relocation fingerprint; raw SHA-256 is
`1a0d58afcbdc2542244c2f23e02ecf89ec7a3d5d88cfc2b63e1518dca66f44d8`.

The exhaustive surviving-object sweep inspected 1,087 candidate objects:
1,086 lacked the function, exactly this dirty object contained it and was
exact, and none was a miss. Its Ninja log records 17 translation-unit compile
entries and no target-specific one-shot history, so that dirty artifact is
source-shape evidence rather than production admission. The present clean
lane performs the first documented bounded production recovery and the only
semantic rename.

## One-shot and policy boundary

The HCEA-named readable typed C89 body and the symbol-table rename were frozen
together before the production compile. A Ninja dry run exposed exactly one
Object Lights `CL` edge plus the target `csplit` refresh. That source compiled
once and was immediately strict. No alternate constant spelling, parameter
name, expression tree, body retry, tuning, compiler flag, comparator exception,
or object manipulation was tested.

The retained function has one parameter per line and an explicit return. It
contains no assembly, `volatile`, `register`, pragma, intrinsic, attribute,
barrier, raw address or offset access, pointer/integer reconstruction,
representation pun, undefined behavior, synthetic anchor, private calling
convention, or byte forcing.

## Ownership and regression boundary

Direct hardened comparison proves all 11 inherited Object Lights functions
remain strict, including `_lights_reconnect_to_structure_bsp` at 96 padded
bytes, seven relocations, and normalized SHA-256
`573e6fdc91ddcdc96231440547fa2cbb6a255d2180b80241ba1a3d02ec86f01d`.

The pre-edit fail-closed translation-unit manifest necessarily reports the
reviewed physical expansion: the deliberate target symbol rename, the new
candidate function and `1.0f` COMDAT owner, and the resulting symbol/debug
inventory changes. It reports `changed_nonexact: []`. No adjudication is
added. Direct comparison of all 12 accepted functions, the full build and
reports, and a clean committed-state snapshot/replay are the acceptance gates
for this additive wave.

## Validation

- Original January split `object_lights.obj` SHA-256:
  `a398a56409b9cf4555ac93df1c208baf328253fc95d2ee6349b0f9bd175d66e9`.
- Renamed January split object SHA-256:
  `ab9291c53fec827d01afa0f605f4a3b6550e70b3c1b81e9f936e642f478c5f8e`.
- Final rebuilt object SHA-256:
  `8f2c443f49d61752048bcb2848460af486df871dd60b235d62a9fcd488c2bcf3`.
- Direct hardened comparison: 12 exact, zero residual, 31 unwritten; all 12
  named comparisons report `all_equal: true`.
- Full Halo and libcmt builds, ordinary report, semantic report, and progress:
  pass.
- Semantic audit: 470 units, 4,871 functions evaluated, 4,744 semantic exact,
  129 hidden exact / 76,388 hidden bytes, 4,754 accepted exact, and zero unit
  errors.
- Aggregate progress: 382/833 linked files, 4,715/11,060 code functions, and
  597,632/2,198,102 meaningful code bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked audit: 12 active, zero stale, and zero invalid.
- Tooling suite: 205/205 tests pass.
- `build/report.json` SHA-256:
  `18c1e675c5c8e48eebaae8e4f4cc951d09916bc63db50fdca99f1dc9d3caa0ad`.
- `build/semantic_report.json` SHA-256:
  `ac1588b0a3e77690cb42bad034bbfd98d0d5419a24ff47b5a8480e298dcf5622`.
- `git diff --check`: clean apart from Git's line-ending notices.
