# `path_obstacles.obj` Jonas constructor-leaf wave

## Result

This bounded source-only wave starts from canonical commit
`710917245462b764326ae6396ef0981a1ad7e9fa`. The one requested body was
emitted in one natural compile and is independently strict.
`path_obstacles.obj` advances from 0/16 to 1/16 exact functions and from
0/3,367 to 21/3,367 meaningful code bytes. It remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_obstacles_new` | 21 | 32 | 0 | `e036552406a449147815d47782ce656a829fa32c049a11f598a67e93cd40eeea` |

The hardened COFF comparator proves identical padded bytes. The function has
no relocations. The other fifteen target functions and all 2,472 target
non-code bytes stay outside this wave's credit. No previously accepted
function existed in this translation unit.

## Provenance and layout

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `path_obstacles.obj` SHA-256:
  `8d8a2106cce26a6c271939867ccb6088d9a132ac4f7ddf8cd7c749489e329bc3`.
- January PDB labels `_obstacles_new` at RVA `0x503F0`. January COFF reads
  one ordinary pointer argument and stores signed zero words at +0x00, +0x02,
  and +0x04 before returning.
- Clean HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its natural constructor body
  blob is `83274a83efab8ffecfb29728424caab8299dce80`; its authenticated
  `obstacles` and `disc` layout blobs are
  `06beab24cdcd5bce62bdd3a85920fc2d3234a9e3` and
  `ffda55f18ef2f47711aeeea4d3a853b26d62ce21`.

The authenticated `struct obstacles` has three signed-short counters at
+0x00/+0x02/+0x04, two reserved bytes, and 128 typed 0x14-byte discs at +0x08,
for a natural total size of 0xA08. Each disc contains signed-short flags and
obstacle index, a 32-bit object handle, a typed 2D point, and a real radius.
The source-local definitions reproduce that complete semantic layout and use
compile-time size assertions; they are not byte-backed offset views.

## Defined-C and one-shot boundary

The retained body is the direct typed operation:

```c
void obstacles_new(
	struct obstacles *obstacles)
{
	obstacles->obstacle_count = 0;
	obstacles->disc_count = 0;
	obstacles->disc_optional_count = 0;
	return;
}
```

The body was compiled once and was strict immediately. There were no rejected
candidates, spelling retries, declaration retries, control-flow experiments,
or compiler-control devices. The source is readable defined C with one
parameter per line and an explicit terminal return. It contains no assembly,
volatile scheduling device, force-inline annotation, optimizer pragma/barrier,
raw address or byte-offset access, pointer/integer reconstruction, pointer or
union pun, inactive-union access, undefined overflow, synthetic anchor,
alignment trick, or object-byte forcing. This translation unit defines no
storage and claims no data credit.

## Scope and validation

Only `source/ai/path_obstacles.c` and this new Jonas-owned ledger are changed.
No shared header, configuration, semantic exception, parked-function record,
protected/frozen source, pre-existing Markdown, Claude-owned file, storage
owner, or deletion is in scope.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: `_obstacles_new` is strict exact at 21/32 bytes
  with zero relocations.
- Semantic audit: 470 units, 4,071 functions evaluated, 3,931 semantic exact,
  3,992 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,979/11,060 exact functions,
  476,554/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Tooling suite: 179/179 tests pass; `git diff --check` is clean.
- The pre-wave fail-closed regression check reports exactly `_obstacles_new`
  as `NEWLY_EXACT`, `changed_nonexact: []`, and the expected symbol-set change
  solely for the new external function owner.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `path_obstacles.obj` is the final same-path reproducibility
proof. No push is performed.
