# `rasterizer_frame_statistics.obj` Jonas lifecycle first shot

## Result

This bounded source-only wave starts from clean cumulative commit
`8fed3d94de39af96077fc8621d2f6f20d8ade9a2`. Both lifecycle bodies match
in the first and only production candidate compile. The object advances from
2/10 to 4/10 strict functions and from 22/5,066 to 108/5,066 meaningful
code bytes. It remains `NonMatching` and claims none of the target's 3,956
data bytes.

| New retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rasterizer_frame_statistics_initialize` | 57 | 64 | 5 | `e6524f7e1703e76d07c98ee107b2be2eafab8e2eb4841d3099768910bec9f8fe` |
| `_rasterizer_frame_statistics_dispose` | 29 | 32 | 3 | `eaf46f3f422fe5af2ff154819db005995baa0678a3d325df95eb9d97d4a9fbee` |
| **New strict total** | **86** | **96** | **8** | |

The initialize relocations are DIR32 to the exact source-path string at
`+4`, REL32 to `_debug_malloc` at `+18`, DIR32 to
`_bss_00466320+0x740` at `+28`, DIR32 to the exact out-of-memory string at
`+35`, and REL32 to `_error` at `+42`. Dispose has DIR32 to the same BSS
member at `+1`, DIR32 to the source-path string at `+15`, and REL32 to
`_debug_free` at `+21`.

## Provenance and documentation audit

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/rasterizer/rasterizer_frame_statistics.obj`: 23,922
  bytes, SHA-256
  `48452628b2525d642bacbb87d89e273e710734f1ca73a5be722e288b4aa4b795`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Current Claude/HCEA working repository commit:
  `afc14e5b1d016560ac5808a7da5aa9f2adb2b885`. Its
  `src/halo/rasterizer/rasterizer_text.c` blob is
  `e31b75db71a6244465284151d73601ffedfd25e9`, and its physical SHA-256 is
  `d131e38d17dcbcd7284803d3a8f2d41474c30f0ff26fec8e19c1208982d868fb`.
- Current HCEA research-cache commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`. The corresponding donor blob
  is `5368528047e6eeb7dc51f21d15726dda85d2c6ac`, with physical SHA-256
  `66ce21f257e67925ff27075c3bbc5b1cc13af935ab36f20c4fe2203d88684de8`.
- The separate data-research repository at commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c` has later-build
  `src/data/rasterizer_frame_statistics.c` blob
  `d29b15beb8549206370f2a76e4e2304014cfd29f`, physical SHA-256
  `248fb3d6527187f1de11f36077778fcc04af1afe64d2b5b4e6e87281370298c1`.
  It documents a different build's small data record and was not used to
  infer January storage ownership.

January COFF proves both new symbols are external cdecl functions, their
complete padded extents, the boolean return in AL for initialize, and every
ordered relocation identity above. Current callers agree with
`boolean(void)` initialize and `void(void)` dispose. Constants and current
macros prove an allocation of `sizeof(word) * 24576 * 3 == 0x24000`, with
`FALSE`, the source path, and line 41 passed through `match_malloc`.

The already-strict `_rasterizer_memory_pool_initialize` is a same-toolchain
source and code-shape donor for the initialize success/failure schedule. The
Claude/HCEA donor `FUN_0017ff60` independently preserves the dispose topology:
load the buffer once, test it, and call `debug_free` with the exact source
path and line `0x345` (837), without clearing the pointer. Its raw address and
donor-build names were rejected; production uses a declaration-only typed
prefix authenticated by January relocation addends.

All available Claude documentation relevant to this object and toolchain was
reviewed before mutation. The canonical `work/halo/CLAUDE.md` and its lane
copies are 47,280 bytes with SHA-256
`c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`;
there is no separate rasterizer-lane variant. The research-cache
`CLAUDE.md` is 37,778 bytes with SHA-256
`97d4c63b9dd9d642cbe6774c5f3a578c60f2de002fce3ac888cd4ee635cc4c3d`.
Their binary-first, C89, ABI, and narrow-validation guidance was applied as
research discipline. Donor-only hooks, commands, raw-address techniques,
configuration, and completion labels were not treated as production rules.
The current campaign's hardened relocation comparator, whole-TU regression,
semantic-progress, admission, and park tools were used as validation
infrastructure, never as matching authority or an exception.

The complete reachable campaign history and registered production-worktree
census revealed no retained definition of either lifecycle function at the
frozen cumulative head. Earlier occurrences were declarations, callers,
target metadata, donor research, or isolated historical experiments, not a
production definition that could be transplanted without fresh proof.

## Retained source

The baseline C blob is
`3577475ca522f832ca4e84c259e5ac7b4f9a5002`; the retained C blob is
`a66aa545fcb0cef1f21018a1a96189e49530a5e6`. Its canonical CRLF worktree
form is 7,071 bytes with 225 CRLF endings, zero bare LF endings, and SHA-256
`c4418711203b3216d852902ee13d038e96aa36c0302d9e7f63d09283d688474c`.
The public header advances from blob
`50abf28f18e1637ed4263e8a6cb1adf402bf3f73` to
`269e89c6961a10665a52199bda510734708b60b9`; its canonical CRLF form is
19,782 bytes with 630 CRLF endings, zero bare LF endings, and SHA-256
`3d9a383981875e1abac6672c0af8b07eb6b45ec185ca3a1c332efe8f250b2e77`.

The C file adds `errors.h`, a translation-unit-only declaration prefix,
one compile-time offset check, one external owner declaration, the lifecycle
bodies, and no storage definition:

```c
struct rasterizer_frame_statistics_private_globals_prefix
{
	byte reserved000[0x740];
	word *temp_buffer;
};

typedef char verify_rasterizer_frame_statistics_temp_buffer_offset[
	offsetof(
		struct rasterizer_frame_statistics_private_globals_prefix,
		temp_buffer) == 0x740 ? 1 : -1];

extern struct rasterizer_frame_statistics_private_globals_prefix bss_00466320;
```

The full 0x860-byte target BSS is deliberately not modeled or defined. The
field is `word *` because the allocation width, triangle/vertex cardinality,
and consumers authenticate that type. Initialize preserves the exact
success-local schedule and silent OOM error. Dispose loads the member once,
conditionally frees it at line 837, does not null it, and explicitly returns.
The header adds only the two public prototypes in target order around the
existing begin/end declarations.

The retained source is readable C89 with `void` on its own no-argument line,
each parameter on its own line, and explicit terminal returns for void
functions. It contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, compiler control, barrier, raw address or raw offset
access, pointer/integer reconstruction, cast or union pun, undefined
behavior, synthetic anchor, byte forcing, data definition, or comparator
exception.

## Failed static-count shot retained as negative evidence

Before this lifecycle wave, a separately frozen attempt at
`_rasterizer_frame_statistics_count_static_vertices` was made once in
`rasterizer-frame-statistics-static-count-20260826`. Its syntax-only check
passed, its baseline stayed unchanged, and its sole production artifact is
1,390 bytes with SHA-256
`086bf822e358868aa633e1cc76e76c82dd93d04063a972c13374acee74b8d757`.
The candidate function normalized to
`df987f2500b4f059b29f638e8a3c6bc588dbbaf1410573fbac13cff295c3a60d`
instead of target
`abb0cdbbf190c800ba8155c28e0a784c40db9fd088fd6ff879704b736ecce18c`.
The target tests the triangle pointer and branches before saving ESI, while
VC7 hoisted the candidate's ESI save ahead of that branch, changing the early
exit pop/NOP topology.

No spelling or control-flow tune was attempted. The exact inverse source
patch was applied, both source blobs were restored, the failed worktree is
tracked-clean, and nothing from that shot entered this wave or the cumulative
branch. The artifact remains preserved as fail-closed evidence.

## First-shot discipline and strict comparison

The clean cumulative baseline object is 1,158 bytes with SHA-256
`b8be8d4817aa61f998b95b49dedd5ab637f2e82608d657f13ab0d6aa3d7c963c`.
The pre-wave whole-TU manifest is 531,236 bytes with SHA-256
`9157c9321d4d87a454e82675ad41e368c0cb3805f6360e698060199322bac5ee`.

The frozen source passed XDK `/Zs` with production include paths and flags;
the restored baseline artifact's size and hash remained unchanged. Its
resolved path was proved inside the isolated worktree, the literal object
was deleted, absence was verified, and the dry run named one edge. The
ordinary selected-object edge then ran exactly once:

```text
[1/1] CL build\base\source\rasterizer\rasterizer_frame_statistics.obj
rasterizer_frame_statistics.c
```

The preserved first artifact is 2,322 bytes with SHA-256
`cee40cf4833fd6b1ea63f0c6c0a13a49e7f8135f01a3448c9c38a53adf234faa`.
There was no declaration, layout, spelling, control-flow, source-shape, or
compiler-tuning retry. CRLF normalization after preservation changes no
preprocessing token or Git blob and is not a code-producing experiment.

Direct hardened comparison proves both new functions and both inherited
functions independently strict:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rasterizer_frame_statistics_initialize` | 57 | 64 | 5 | `e6524f7e1703e76d07c98ee107b2be2eafab8e2eb4841d3099768910bec9f8fe` |
| `_rasterizer_frame_statistics_begin` | 21 | 32 | 2 | `1b0bb2d783bacbf754a36437dbe7d3d99a8df5869a3dbd473bb67437426572d0` |
| `_rasterizer_frame_statistics_end` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_frame_statistics_dispose` | 29 | 32 | 3 | `eaf46f3f422fe5af2ff154819db005995baa0678a3d325df95eb9d97d4a9fbee` |
| **Strict total** | **108** | **144** | **10** | |

The generic pre-wave gate reports the two lifecycle bodies as
`NEWLY_EXACT`, an empty `changed_nonexact` array, and expected COMDAT/debug/
symbol-owner index changes. It cannot carry begin/end through their old
section indices after the new external COMDATs and string COMDATs are
inserted. This is reviewed evidence, not a waiver: direct hardened comparison
proves both inherited complete padded sections, normalized bytes, and ordered
relocations still exact. No adjudication, semantic exception, or comparator
change was added.

The first artifact defines exactly the four table functions. Undefined
function externals are `_error`, `_debug_malloc`, `_csmemset`, and
`_debug_free`; undefined data externals are `_bss_00466320` and
`_rasterizer_frame_statistics`. It retains the baseline's 8-byte COMMON
`_global_frame_parameters` and defines only the two exact string COMDATs as
new runtime non-code externals. It defines no BSS owner and receives no
matched-data credit.

## Validation

The complete Halo and libcmt object graph passes. Canonical outputs are:

- `build/report.json`: 1,574,621 bytes, SHA-256
  `3389a64d1524f29a1085cba628981ecb8d12becfbdd8af9cd1b03a97c9d6a8c6`;
- `build/semantic_report.json`: 2,982,992 bytes, SHA-256
  `fbf4a39b5d4e568a859bfc60cd676560896dd5fb7e0397b78963135e367e5119`.

Campaign progress becomes:

- all categories: 375/833 complete objects, 4,154/11,060 exact functions,
  502,536/2,198,102 meaningful code bytes, and
  1,835,208/4,176,062 data bytes;
- Halo: 273/468 complete objects, 3,987/7,574 exact functions,
  489,622/1,770,166 meaningful code bytes, and
  1,830,016/3,923,451 data bytes;
- libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The strict semantic audit scans 470 units and evaluates 4,261 functions:
4,119 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36
ordinary-only, 35 structural, one rejected, 12 local skips, 3,468 missing,
4,180 accepted exact, and zero unit errors. Admission remains zero candidates
and zero revocations, with only the inherited unrelated
`source/shell/shell_xbox` completion-label contradiction. Parks remain three
active, zero stale, and zero invalid. All 179 tooling tests pass, and all 14
selected tracked/configuration/canonical-report JSON documents parse.

The tracked scope is exactly
`source/rasterizer/rasterizer_frame_statistics.c`, the two public declarations
in `source/rasterizer/rasterizer.h`, and this ledger. There are no deletions.
The complete `source/units/**` tree and all eight protected paths have zero
intersection with the wave. No push, amend, rebase, history rewrite, or
worktree removal is performed.
