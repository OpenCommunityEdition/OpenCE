# `path_obstacle_avoidance.obj` Jonas two-leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`ffd8962fce5b766982c1d6fb32a70d8a8cd87b8c`. The two authenticated ordinary-C
bodies were emitted together in exactly one natural candidate compile. Both
were independently strict on that first emission and are retained without a
spelling, declaration, layout, or compiler-control retry. The inherited
`_heap_left_index` and `_heap_right_index` bodies also remain strict.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_path_get_step` | 75 | 80 | 4 | `ae2df3005e2c2fb9bb7cff93e5e556aa251e7849df4c31272f2f5ed524ed15b5` |
| `_heap_parent_index` | 51 | 64 | 4 | `d881211c22ac87e87eea84ff27ee5a869b529d6deb342183bb588083d59e7eff` |
| **Wave gain** | **126** | **144** | **8** | |

Together with the two inherited heap-index leaves, the accepted subset is 150
meaningful bytes, 176 padded bytes, and eight relocations. The object advances
from 2/24 to 4/24 exact functions and remains `NonMatching`. Assertion literals
receive zero data credit.

## Provenance and signatures

- Baseline `source/ai/path_obstacle_avoidance.c` blob:
  `00920e103f83b2625025b22064965d08edf9fec3`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `path_obstacle_avoidance.obj`: 28,413 bytes, SHA-256
  `773fcf3c4c50847f26051520cb99edd537ecbc8d810752be010d233acc009303`.
- Clean HCEA source oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Relevant tree blobs are
  `f583b65e74fb286a0f8cf112ad31ab721bd6144b` (`path_get_step.c`),
  `04370106c8187e453773b665b65b5ced43764556` (`heap_parent_index.c`),
  `019f323f786eba8c2a3d4c06087d9444b3630120` (`obstacle_path.h`), and
  `129512bb3d81c656f3f41ba877f929022571a84c` (`step.h`).
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Generated flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 and include paths.

HCEA supplies the typed source topology and these semantic signatures:

```c
struct obstacle_path_step *path_get_step(
	struct obstacle_path *path,
	short step_index);
short heap_parent_index(
	short heap_index);
```

HCEA calls the 0x28 leaf record `step`; this translation unit gives the
source-local tag the unambiguous name `obstacle_path_step`. Tag spelling does
not cross the C ABI. January COFF defines `_path_get_step` and
`_heap_parent_index` as external type-0x20/storage-2 symbols. Their stack
argument loads and plain `ret` instructions independently prove ordinary
`__cdecl` ABI rather than a private register convention.

## Typed layout and accepted behavior

The source-local `obstacle_path_step` and `obstacle_path` records are readable
typed copies of the clean HCEA layouts. Compile-time assertions prove the step
size `0x28`, path size `0x1534`, `step_count` at `+0x2C`, `steps` at `+0x30`,
`heap_count` at `+0x1430`, and `heap` at `+0x1432`. January instructions
independently load the signed count at `+0x2C` and compute
`path + 0x30 + step_index * 0x28`.

`path_get_step` uses the January-authenticated assertion
`step_index>=0 && step_index<path->step_count &&
path->step_count<=MAXIMUM_OBSTACLE_AVOIDANCE_STEPS`, with maximum 128,
source path `c:\halo\SOURCE\ai\path_obstacle_avoidance.c`, and source line 40.
It then returns the addressed typed array element.

`heap_parent_index` uses the authenticated `heap_index>0` assertion with the
same source path and source line 57, then returns
`(short)((heap_index - 1) >> 1)`. The assertion constrains the arithmetic to a
positive input. Both bodies are the direct HCEA expressions augmented only by
the assertion control flow and constants fixed by January COFF.

The strict comparator proves identical padded bytes and all relocation
identities. Each function has one `DIR32` relocation to the compiler-emitted
file literal, one `DIR32` relocation to its compiler-emitted expression
literal, and `REL32` relocations to `_display_assert` and `_system_exit`.

## One-shot boundary and policy

The fresh branch and worktree started at the exact authoritative commit. Before
emission, target splitting and COFF inspection were read-only,
`git diff --check` was clean, and an MSVC `/Zs` parse-only invocation accepted the fixed
two-body source without producing an object. The candidate object did not
exist. One code-producing compile then emitted both bodies together. Its
1,868-byte object SHA-256 is
`08a097751280b745a9e174605ca6014e8eb232d364118737882c83ce71ad0fb4`.
Both new functions and both inherited functions were strict immediately, so no
candidate was removed and no retry occurred.

Only `source/ai/path_obstacle_avoidance.c` and this new Jonas-owned ledger are
changed. No shared header, protected AI source, configuration, semantic
exception, parked record, global storage, COMMON owner, `.bss`, or manually
owned `.rdata` is added. The assertion strings are ordinary compiler-emitted
function support and receive no data claim.

The source contains no assembly, raw address, byte-offset dereference, casted
overlay, pointer pun, inactive-union read, undefined overflow, `volatile`
scheduling, force-inline annotation, optimizer pragma/barrier, synthetic
codegen anchor, or object-byte forcing.

## Validation before commit

The complete 569-action `halobetacache_build` and `libcmt_build` set passes
(the one candidate object plus the remaining 568 build actions).

- Strict semantic audit: 470 units and 4,146 functions evaluated; 4,006
  semantic exact, 101 hidden exact / 61,876 hidden code bytes, 4,067 accepted
  exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 4,054/11,060 functions,
  481,985/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 data bytes.
  Halo contributes 3,887/7,574 functions and 469,071/1,770,166 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- Direct hardened COFF comparison passes all four accepted functions.

## Committed source payload

The source payload to be committed is Git blob
`fcd39d015449806d05d920911297cd676fb84076`, 5,822 bytes, with SHA-256
`7ea36eca4b9822ecf30aea4c1f6c5b546eda6aacfc7cf411f228a45ad70bc2f5`.
It is the only changed source file. The committed-state object deletion and
normal rebuild replay is intentionally recorded in an additive ledger-only
commit after this source-plus-ledger commit; history will not be amended.
