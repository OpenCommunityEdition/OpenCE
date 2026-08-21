# `glow.obj` Jonas parametric-line leaf

## Result

This bounded source-only wave starts from authoritative integration commit
`5c14f9595e860b598a2dfc49631cc6f3c4534664`. One authenticated ordinary-ABI
body was emitted in one natural XDK candidate compile and was strict
immediately. There was no body-shape change, compiler-control experiment, or
candidate retry.

`glow.obj` advances from 4/25 to 5/25 exact functions and remains
`NonMatching`. The wave gains 47 meaningful and 48 padded code bytes with no
relocations and no data credit.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_point_from_parametric_line` | 47 | 48 | 0 | `6c4822844854ba7276467fc1bd1342d78de539c903ab04e909044e2137d77020` |

The hardened COFF comparator reports `all_equal: true`. The padded bytes and
ordinary external function ownership are identical to January. The target's
other 20 functions and 6,761 meaningful code bytes remain outside this wave's
credit. All four pre-existing exact functions in the object also remain
strict under direct comparison.

## Provenance and one-shot boundary

- Baseline `source/objects/widgets/glow.c` Git blob:
  `3cc3eb64f539bf604ddb7176835ed7056e358e88`.
- Retained source Git blob / stable committed-payload SHA-256:
  `c80a967210a0770babc49e75b6b39b5c51b43cb4` /
  `5479e52c7f1886cdf40114ca626c3b8c9ff250445b4df805f28dd99eead7c6d3`.
  The SHA is over `git cat-file blob` payload bytes, not checkout EOLs.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `glow.obj` SHA-256:
  `593db0e7fde065d7426231e9e08aaed6360b790033077481e1d70044e54a638a`.
- The clean HCEA semantic oracle is commit
  `c168af2e747d3095d9a29418ae401f3a39544863`; its readable function-source
  tree blob is `c6267cfc227f031e8d7764ed915c39defc842cc6`. That source independently
  authenticates all three component assignments, including the historical
  final addition of the input point's `y` component. Its PPC-facing extra
  parameter is a platform ABI artifact and is not imported into the PC body.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  using the repository's unchanged XDK 3911 command and flags.

January's PDB label, file boundary, and x86 body fix the PC signature as:

```c
void point_from_parametric_line(
	real_point3d const *point,
	real_vector3d const *forward,
	real t,
	real_point3d *result);
```

The January function starts at executable file offset `0x122D40`, consumes
exactly four cdecl stack arguments, and returns with an ordinary `ret`. Exactly
one new object-producing candidate compile covered the body. It passed on that
first object. Later normal and forced same-path compilations were validation
replays of the unchanged retained source, not alternate candidate searches.

## Authenticated typed behavior

The helper writes the three components of `result` directly. It computes
`forward->i * t + point->x`, then `forward->j * t + point->y`, then
`forward->k * t + point->y`, and returns explicitly. January independently
confirms the unusual final `point->y` load at input offset `+0x4`; the clean
HCEA source records the same behavior, so the asymmetry is authenticated
engine semantics rather than target-derived source tuning.

The source uses the repository's typed `real_point3d` and `real_vector3d`
members. January's x86 reads the input point at `+0x0/+0x4`, the direction at
`+0x0/+0x4/+0x8`, and writes the result at `+0x0/+0x4/+0x8`. It emits no call,
global, literal, or other relocation.

## Data and ownership boundary

The retained helper owns only its 48-byte COMDAT code section. It introduces
no string, float COMDAT, initialized data, BSS, COMMON symbol, writable
storage, or external data owner. The candidate object's pre-existing non-code
sections and ownership remain frozen by the whole-translation-unit regression
gate. The target's 392 aggregate data bytes therefore remain wholly outside
campaign data credit for this partial unit.

## Policy, scope, and validation

Only `source/objects/widgets/glow.c` and this new Jonas-owned ledger change.
No header, configuration, semantic exception, parked record, status or
completion label, pre-existing Markdown, Claude-owned file, protected/frozen
source, or tracked-path deletion is involved.

The implementation is readable typed C with one parameter per line and an
explicit terminal `return;`. It contains no assembly, raw address or byte-
offset access, pointer/integer reconstruction, cast or union pun, inactive-
union trick, undefined signed overflow, `volatile` scheduling device, force-
inline annotation, optimizer pragma/barrier, synthetic anchor, or object-byte
or code-generation forcing.

- Complete `halobetacache_build` and `libcmt_build`: passed.
- Direct hardened comparison: the new helper and all four pre-existing exact
  functions passed.
- Semantic audit: 470 units, 4,089 functions evaluated, 3,949 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,010 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,997/11,060 exact functions,
  477,968/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave added one function and 47
  meaningful code bytes with no data increase.
- Halo progress: 273/468 complete objects, 3,830/7,574 exact functions,
  465,054/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remained.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests passed; `git diff --check`, deleted-
  path, protected-scope, and banned-policy scans were clean.

After the source and ledger were committed, a clean committed-state regression
snapshot/check passed. The built `glow.obj` was then deleted and forced through
the normal same-path Ninja edge; the post-rebuild regression check and direct
hardened comparison again passed all five exact functions, reproducing the new
helper's padded size, normalized hash, zero-relocation shape, and non-code
ownership boundary. The raw object-container SHA is deliberately not a
reproducibility criterion because VC7 writes the compile time into the COFF
header. The committed source blob and stable payload SHA above were reproduced
from `HEAD`. The final worktree was clean. No push was performed.
