# `path.obj` Jonas input-leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`85491a7c029faa3bdfa8154ec214a1924815ddd6`. Five HCEA-backed
`path_input` bodies were emitted together in one natural candidate compile.
Three were independently strict and are retained; the two nonexact bodies
were removed immediately without a spelling, declaration, field-order, or
compiler-control retry. The four pre-existing exact lifecycle leaves remain
strict. `path.obj` advances from 4/32 to 7/32 exact functions and remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_path_input_new` | 47 | 48 | 1 | `04cfc3fddcd18bed87e0d9c79c1ad355679f4599e6528b8e42f3c8ab72da4e0c` |
| `_path_input_set_target_object` | 14 | 16 | 0 | `d7de9ac73bd916e7fa8ee4115138af50f9dda920f19ebef3d3968c0754c1a202` |
| `_path_input_set_start` | 42 | 48 | 0 | `2450ed1f92d90ef9bf0af9009cd11b41abcbff9444e87386ec18143bde59413c` |
| **Wave gain** | **103** | **112** | **1** | |

Together with the four one-byte lifecycle leaves, the final accepted subset
is 107 meaningful bytes, 176 padded bytes, and one relocation. The hardened
COFF comparator reports identical normalized bytes, padding, relocation type,
offset, destination, and addend for all seven functions.

## Provenance and signatures

- Baseline `source/ai/path.c` blob:
  `7496e61b770fbf67829f4ef9822e76bc7f313898`.
- Unchanged `source/ai/path.h` blob:
  `e1cce6da92727aeefeec6b2fbe756a0d01d5df27`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `path.obj` SHA-256:
  `1b989e4dc3bf3d9623cec07fa19164794044eb0bf359cf08480cf0ea8b3a709c`.
- Extracted January PDB-symbol JSON SHA-256:
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
  It independently fixes all five public symbol names and their RVAs.
- Clean HCEA source oracle commit:
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`. Relevant blobs are
  `4adff000aa4e4c0e516a6ac4bd6f85214db4de2b` (`path_input_new.c`),
  `0b2e3858fbe991e55379201849e80bebddf81b18`
  (`path_input_set_target_object.c`),
  `e46fa002abb0344a4cbd9d3f5da26448bcb30871` (`path_input_set_start.c`),
  `e47a23ad982f7af77cd0f05089013798856ac476`
  (`path_input_set_attractor.c`),
  `5ad8b433fca567cbcc4d02eceb1c0c83c4ea7797`
  (`path_input_set_search_bounds.c`), and
  `e2f573d9b2aaea8dd0c384ffbe0c14dae870d2af` (`path_input.h`).
- Preserved January-PC source oracle commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its `path.c` blob is
  `398d2a345bbbd20eded999a7dde8a337a68c6ac6`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler
  `13.00.9254.1` for 80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Generated flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 and include paths.

HCEA supplies the typed bodies and establishes these signatures:

```c
void path_input_new(
    struct path_input *input,
    real pathfinding_radius,
    boolean ignore_broken_surfaces,
    long ignore_source_object_index);
void path_input_set_target_object(
    struct path_input *input,
    long target_object_index);
void path_input_set_start(
    struct path_input *input,
    const real_point3d *start_point,
    long start_surface_index);
```

The HCEA header and January-PC codeflow independently corroborate the 0x48
record, byte-sized Boolean fields, full 32-bit object/surface handles, and
the `real_point3d` copy. January COFF is authoritative for exact field
offsets and code extent. Source-local compile-time assertions prove size
`0x48` and every field observed by the retained bodies: `+0x04`, `+0x08`,
`+0x0C`, `+0x10`, `+0x14`, and `+0x20`.

## Accepted behavior

`path_input_new` clears exactly 0x48 bytes through the ordinary typed
`csmemset` prototype, installs the radius and broken-surface setting, records
the source-object handle, and initializes the target-object handle to
`NONE`. Its sole `REL32` relocation is to `_csmemset` at `+0x0D`.

`path_input_set_target_object` writes the complete 32-bit datum handle at
`+0x0C`. `path_input_set_start` marks the start valid, copies the three-real
point to `+0x14`, and stores the full surface index at `+0x20`. All three
functions use named fields, one parameter per line, and an explicit terminal
`return;` required by house style.

## One-shot rejection record

| Rejected function | Target meaningful / padded / relocs | Target hash | First candidate hash | First natural divergence |
| --- | ---: | --- | --- | --- |
| `_path_input_set_attractor` | `54 / 64 / 0` | `a2381416ca8eb7260cc39f620afc25c596184e843e39d8d0d6a79b2bc52207e9` | `66edc5785ddb33f93d747414ebe77604a3bc5dd1f2f1c6fed61ad5348b0fdfb9` | HCEA's explicit component assignments emit direct field stores with no saved register; January uses an aggregate-copy schedule with `ESI`. Size and dependency shape agree, but bytes do not. |
| `_path_input_set_search_bounds` | `18 / 32 / 0` | `4b43e0db3f70c64a1707806861a0d3fe476a0f7b32b2c7161af16fcdf78c9b43` | `c289eceac8795d20c1090052a1badb5723acf58d18c50f6a5e116be5f5970961` | HCEA's value-first assignment emits an x87 load/store followed by the flag; January bit-copies through a GPR and writes the flag first. |

Both rejected COMDATs and their support-only offset assertions are absent
from the final object/source. Do not retry an aggregate-copy spelling,
assignment reversal, scalar-type substitution, declaration permutation, or
other disassembly-guided codegen variant. Reopen only with an independently
preserved source artifact that proves the original January source topology.

`_path_state_new` was never emitted. January clears `0x1408C` bytes, whereas
the current/HCEA shared `struct path_state` measures `0x1008C`; accepting it
would require an out-of-scope shared-header correction or source-local filler.

## Policy, ownership, and data boundary

Only `source/ai/path.c` and this new Jonas-owned ledger are changed. No shared
header, configuration, frozen source, semantic exception, parked record,
pre-existing Markdown, or Claude-owned file is modified. The source defines
no global storage and adds no `.bss`, `.data`, COMMON owner, or runtime
`.rdata`. No part of the target's 2,608 non-code bytes is claimed.

The retained source uses ordinary typed objects and named fields. It contains
no assembly, raw address, byte-offset dereference, cast, pointer arithmetic,
type pun, inactive-union read, undefined overflow, `volatile` scheduling,
force-inline annotation, optimizer pragma/barrier, synthetic codegen anchor,
or object-byte forcing.

## Validation

The final `path.c` blob/SHA-256 is
`2615961b02b3aa303916f26f1aa1dc3c6fcff92f` /
`0ba93c4888d46293a55df799fde90717e421194a338bd1fbe4adc89a47d85c27`.
The complete 569-action `halobetacache_build` and `libcmt_build` pass.

- Strict semantic audit: 470 units, 4,018 functions evaluated, 3,878
  semantic exact, 101 hidden exact / 61,876 hidden code bytes, 3,939 accepted
  exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,926/11,060 functions,
  474,289/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 3,759/7,574 functions and
  461,375/1,770,166 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.
- The fail-closed pre-wave translation-unit check identifies exactly the
  three retained functions as `NEWLY_EXACT`, preserves all four lifecycle
  leaves as `still_exact`, and reports `changed_nonexact: []`. The pre-wave
  manifest was captured after canonical advanced to `d90cd762`, but that
  commit retains the authoritative baseline `path.c` blob and four-function
  object unchanged. The only generic structural findings are the expected
  `.debug$S` and symbol-set changes from adding three COMDATs; no adjudication
  or configuration exception is added.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `path.obj` is the final same-path reproducibility proof. No push
is performed.
