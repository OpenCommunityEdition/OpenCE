# `structure_render.obj` Jonas fog-offset leaf

## Result

This bounded source-only wave starts from authoritative integration commit
`017d277f79e3984738658b698fab2c896eac45d5`. The sole requested public
ordinary-cdecl body was emitted in exactly one natural code-producing
candidate compile and was strict on that first emission.

`structure_render.obj` advances from 0/19 to 1/19 exact functions and remains
`NonMatching`.

| Retained function | RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_structure_render_set_fog_offset` | `0x00184D30` | 78 / 80 | 9 | `584039932866464d123170bc2c3aee2302c3fd610bfd21f266ab4e82f59a5ce9` |

The hardened comparator reports identical normalized bytes, padded extent,
and relocation address, type, symbolic destination, and addend. The other
eighteen target bodies, all target writable storage, and all 19,050 target
non-code bytes remain outside this leaf's matching credit.

## Provenance and Git payload authentication

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `structure_render.obj` SHA-256:
  `113539484a7eaacc6eafa639163888842e5ad2e4d3ca181ee2d2fd659c7dc503`.
- First and only candidate `structure_render.obj` SHA-256:
  `ff18405b73a37ce712eb780744534b78e4282de0ed54920116d3e99674ec2d25`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Clean HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`.

Every Git identity below was read as its raw blob payload. The byte count and
SHA-256 authenticate that payload independently of the Git object ID.

| Evidence | Git blob | Bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| baseline `source/structures/structure_render.c` | `8ca4b9df8006de6e8cb161fd9bafcc31b66873ff` | 2,709 | `34a07e8642c3037b7ccf4e9d0c3fb898e9aa58388ab072970432de07b5fd07c8` |
| retained `source/structures/structure_render.c` | `95cd4a11785342bd098a4983c5a3aac99bed5138` | 4,234 | `588ac1f422d362a77e66ed53caad9b29b130b9c732207c2999a208cf8046c3c8` |
| canonical `source/structures/structures.c` caller evidence | `6df8338b6db46ce5653aac68150a280bb4601220` | 2,364 | `daf1c271cb1a309d5b7574e393ef72ce2baba70e270365bb9312c1563e9610e4` |
| canonical `source/cseries/cseries.h` width/type evidence | `3c91ac46ab275894a18ac4e839b38ae13022d91b` | 10,070 | `e994e965cd3f48cd47cebf0a39f41a1aac460e8b25544452a1fc4b302a0b7043` |
| canonical `source/math/real_math.h` vector evidence | `f14567675126171a4b1c2d3052e8058c68c029c2` | 43,695 | `c5b4659b368565ff7db2f51536b631694d5db9df01631be8cc1385934e51cf01` |
| canonical `source/render/render.h` assertion evidence | `acc338385e60f02f639532a0e550072faab47f9a` | 3,036 | `f80c48be68ce216458b482b47427893ac53a59ca9938b597f802735e9359edd1` |
| canonical `source/structures/structure_render.h` | `69f73dcb2ac4c3b35aa81a4b12ab5eef0f70db4d` | 370 | `de5700c86db170d70aa46ddb1ad82beeeb204d30253bf23bd4f9e7d9f2d19b83` |
| HCEA function donor | `388c8d8612be041c2417c836403d8e9e9ed00c3b` | 3,447 | `e8c83f523a5271e4e9aadc7dda0ba6423b4aeb8b3fe49bb872e7f287a3e50d89` |
| HCEA planar-fog caller | `a6a314940de00365b69e1b7af4faf9b5e5de86d7` | 4,193 | `2eb2eb54ac5cdb65061942298a93426b18440657798e790c2605a3519acf06d3` |
| HCEA structure-render layout | `5e2acca54a4848ad5090ac880b435a5c663e54db` | 934 | `7d615f8bdc21950b6e368563ba23792c95835b85d7e17131a7c55995bbadd10a` |

The canonical source history contains only its initial implementation and an
unrelated console-formatting change. No existing branch, worktree body, or
object-matching ledger claims this leaf.

## Signature, caller, layout, and relocation proof

The authenticated public signature is:

```c
void structure_render_set_fog_offset(
	real_vector3d *vector_offset);
```

January has exactly one direct call at RVA `0x001888FC`, inside
`_structure_get_planar_fog`. The caller constructs a local three-float vector,
uses `lea` to pass its address, and performs caller stack cleanup. The callee
reads the ordinary stack argument at `[ebp+8]` and returns with plain `ret`.
There is no address-taken immediate occurrence. This authenticates the pointer
parameter, ordinary cdecl ABI, and undecorated C identifier that emits the
target COFF spelling `_structure_render_set_fog_offset`. HCEA independently
declares the same signature and passes `&offset` from its planar-fog caller.

The HCEA layout and January opcodes jointly authenticate a TU-local typed
prefix with these named fields:

| Field | Offset |
| --- | ---: |
| `environment_geometry_valid` | `0x00` |
| `environment_triangles_index` | `0x04` |
| `fog_offset_valid` | `0x08` |
| `fog_offset_vector` | `0x0C` |
| prefix size | `0x18` |

Compile-time `offsetof` and `sizeof` proofs enforce every value. The owner is
declared, but never defined, as the direct external object
`extern struct structure_render_globals_prefix bss_004c0cd0;`; the C
identifier emits target symbol `_bss_004c0cd0`.

This direct-object spelling is a documented pre-emission correction. The
preliminary dispatch described `bss_004c0cd0` as a pointer, but January uses
direct absolute object accesses: `A0`/`C6 05` at owner addend `+8`, followed by
direct stores at `+12`, `+16`, and `+20`. A pointer declaration would have
introduced an unsupported load and dereference. The opcode and relocation
evidence therefore governed, and the corrected direct extern object was
explicitly authorized before the sole candidate compile. No storage was
introduced by that correction.

The nine exact COFF relocations are:

- DIR32 `_bss_004c0cd0+8` at `+4`;
- DIR32 source-path literal at `+17`;
- DIR32 assertion-expression literal at `+22`;
- REL32 `_display_assert` at `+27`;
- REL32 `_system_exit` at `+34`;
- DIR32 `_bss_004c0cd0+8` at `+46`;
- DIR32 `_bss_004c0cd0+12` at `+55`;
- DIR32 `_bss_004c0cd0+16` at `+64`;
- DIR32 `_bss_004c0cd0+20` at `+72`.

## Defined-C body and one-shot boundary

The retained source naturally asserts at canonical line 103 that the latch is
clear, sets the latch, and copies `i`, `j`, and `k` explicitly into the typed
vector field. It ends with an explicit terminal return. A macro preserves the
canonical assertion text `!structure_render_globals.fog_offset_valid` while
the named direct object emits the authenticated external COFF symbol.

Exactly one code-producing candidate compile crossed the boundary. It was
strict and retained unchanged. There was no spelling, layout, x87, scheduling,
declaration, compiler-flag, or control-flow retry. A later complete-build
attempt first stopped on an unavailable prerequisite-tool download; no source
or candidate expression changed. Reconfiguration to the already authenticated
local `csplit`, `objdiff`, and Ninja executables allowed the normal complete
graph to finish. That prerequisite recovery was transparent and did not
produce a second candidate compile.

The source is readable defined C with no assembly, raw offset or address,
pointer cast or overlay, type pun, inactive-union access, undefined behavior,
volatile scheduling device, register forcing, optimizer pragma or barrier,
synthetic anchor, or byte-forcing expression. There is no header edit, private
helper, or owned storage.

## Ownership and scope boundary

Only `source/structures/structure_render.c` and this new Jonas ledger change.
Protected `vehicles.c`, `ai_debug.c`, `ai_script.c`, `actions.c`, and `units.c`
are untouched, as are shared headers, configuration, semantic exceptions,
parked records, completion labels, pre-existing Markdown, and tracked-path
ownership.

The final candidate object defines exactly one external code owner,
`_structure_render_set_fog_offset`, and the two canonical assertion-string
COMDATs. It defines no `.data`, `.bss`, COMMON symbol, or aggregate owner.
`_bss_004c0cd0`, `_display_assert`, `_system_exit`, and the compiler marker
`__fltused` remain undefined externals. Every rejected target code symbol is
absent. The two literal COMDATs are required runtime support for the accepted
assertion but receive no data-matching credit; all 19,050 target non-code bytes
remain unclaimed.

## Validation before commit

- One natural candidate compile: strict first-shot acceptance at 78/80 bytes,
  nine relocations, and the required normalized SHA-256.
- Complete Halo and libcmt builds plus generated ordinary and semantic
  reports: pass. Inherited compiler warnings remain confined to unrelated
  translation units; this source emitted no failure or warning.
- Direct hardened comparison: strict equality for the retained code COMDAT;
  every other target code body is absent from the candidate.
- `structure_render.obj`: 1/19 functions and 78/3,380 meaningful code bytes;
  zero data credit; remains `NonMatching`.
- Semantic audit: 470 units, 4,136 functions evaluated, 3,996 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,057 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,044/11,060 exact functions,
  481,108/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 3,877/7,574 functions and
  468,194/1,770,166 code bytes; libcmt contributes 167/476 functions and
  12,914/55,015 code bytes.
- Admission: zero candidates and zero revocations; the one inherited
  `shell_xbox` completion-label contradiction remains explicit.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, source-policy, deleted-path, protected-scope, and
  changed-path audits: pass.

The implementation commit made no advance claim about committed-state replay.
The additive record below supplies the actual clean snapshot, verified
generated-object deletion, normal Ninja rebuild, regression check, direct
comparator, rejected-symbol absence, and final object ownership.

## Recorded committed-state replay

The implementation commit is
`39fa61f819d6d350741492a8ccc1b91bd96df806`. At that clean committed state,
`source/structures/structure_render.c` resolves to Git blob
`95cd4a11785342bd098a4983c5a3aac99bed5138`; its raw committed payload is
4,234 bytes with SHA-256
`588ac1f422d362a77e66ed53caad9b29b130b9c732207c2999a208cf8046c3c8`.
`git status --short --branch` reported only the branch header and no changed or
untracked path.

This committed payload identity also corrects the implementation ledger's
pre-commit measurement. That earlier row combined the filtered Git object ID
with a direct measurement of the 4,327-byte working-tree CRLF payload, whose
SHA-256 is
`3b2060002f75bdebf5a279a14e86bc75ae2504ea14a92b8d18f8e3943de54980`.
The authoritative raw Git-blob payload above is the retained identity. The
correction is ledger-only and additive: source commit `39fa61f8` is preserved
unchanged, with no amend or history rewrite.

The clean regression snapshot was written to
`build/regression_structure_render_fog_offset_20260821.json` with status
`SNAPSHOT_WRITTEN`, commit
`39fa61f819d6d350741492a8ccc1b91bd96df806`, and sole unit
`source/structures/structure_render`. The generated object path was resolved
inside this worktree. Its existing 1,324-byte file, SHA-256
`ff18405b73a37ce712eb780744534b78e4282de0ed54920116d3e99674ec2d25`,
was verified before `build/base/source/structures/structure_render.obj` was
deleted; a second existence check proved it absent. The normal repository
Ninja/VC7 edge then rebuilt that exact object path with the unchanged compile
flags `/O2 /Oy- /DDEBUG /Dxbox`.

The committed-state regression check returned `ok: true`, no failures, no
warnings, no newly exact functions, and no changed nonexact functions. Its
`still_exact` set is exactly `_structure_render_set_fog_offset`. A subsequent
direct hardened comparison again reported strict equality at 80 padded bytes
and nine relocations, including every address, type, symbolic destination,
and addend. All eighteen rejected target code symbols are absent.

The replay object is 1,324 bytes with SHA-256
`1af917bb4221006f764a9ebe420d65b470dbdb6dfcf9327b1010fd8e19218907`.
Its sole defined external code owner is
`_structure_render_set_fog_offset`. Its only defined runtime non-code owners
are the 43-byte assertion-expression literal and 45-byte source-path literal;
there is no writable `.data`, `.bss`, COMMON, or aggregate owner.
`_bss_004c0cd0`, `_display_assert`, `_system_exit`, and `__fltused` remain
undefined externals. The worktree remained clean after replay.

Nothing is pushed or history-rewritten.
