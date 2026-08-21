# `actor_looking.obj` Jonas two-body wave

## Result

This bounded source-only wave starts from authoritative integration commit
`8e6b10c30bc2658f9a89b360cbe03564b7146b41`. Exactly two authenticated,
ordinary-ABI actor-looking bodies were emitted together in one natural XDK
candidate compile. Both were independently strict in that first object and
are retained without a body-shape change, compiler-control experiment, or
candidate retry.

`actor_looking.obj` advances from 0/16 to 2/16 exact functions and remains
`NonMatching`. The wave gains 406 meaningful and 416 padded code bytes with
24 relocations. It does not claim a campaign data increase.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_look_secondary_stop` | 111 | 112 | 7 | `32ede7c321e2b80f80155ee4ea993670fb1c9ec148e2458760ae9ecc0f7d7c7f` |
| `_actor_look_compute_prop_interest` | 295 | 304 | 17 | `33b0547c0151f3f5dc6117e59e8cb81060cc5f431171fd4685a533436006cb714` |
| **Wave gain** | **406** | **416** | **24** | |

The hardened COFF comparator reports `all_equal: true` for both retained
bodies. Their padded bytes and every relocation address, type, symbolic
destination, and addend are identical to January. The target's other 14
functions and 10,428 meaningful code bytes remain outside this wave's credit.

## Provenance and one-shot boundary

- Baseline `source/ai/actor_looking.c` Git blob:
  `d52139ae225fdc51652f9fbc349ba702a52830e1`.
- Retained source Git blob / stable committed-payload SHA-256:
  `7ce5be06a5ee69913d005365384c6e5ecdf61b44` /
  `3a28bbe308c4cc7619bdb6fd12afc25387aa6f8febd6c4c186d1e633d7f34cf6`.
  The SHA is over `git cat-file blob` payload bytes, not checkout EOLs.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `actor_looking.obj` SHA-256:
  `443e269f49461dcaf5ff3804fc8770cc5579798427289c60f56a1ce533a226de`.
- The clean HCEA semantic oracle is commit
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its two readable body blobs are
  `b03b1dc04e840790a7ee83deb4b481eda2cb6e88` and
  `6518634db779fffceb4f3390d7de6e9a5ac02435`. HCEA authenticates the named
  state transitions and interest calculation; January remains authoritative
  for the debug branch, PC signatures, exact code shape, literals, and COFF
  ownership.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  using the repository's unchanged XDK 3911 command and flags.

January's PDB and split object fix the ordinary C signatures as:

```c
void actor_look_secondary_stop(long actor_index);
real actor_look_compute_prop_interest(long actor_index, long prop_index);
```

Their January file offsets are `0x15E80` and `0x15EF0`. The sole other
production declaration is the compatible two-`long`, `real`-return prototype
in `actor_perception.c`; there is no competing definition or ABI conflict.

Exactly one new object-producing candidate compile covered both bodies. Both
passed immediately. Later normal and forced same-path compilations were
validation replays of the unchanged retained source, not alternate candidate
searches.

## Authenticated typed behavior

`actor_look_secondary_stop` obtains the actor through the existing typed
`actor_get` object macro. When a secondary look is active and the typed AI
debug-print byte is enabled, it prints the January `"%s: look-stop"` message
using the existing actor-description and console interfaces. It then clears
the named secondary-look priority, type, and timer fields and returns
explicitly.

`actor_look_compute_prop_interest` obtains the actor and prop through the
typed `actor_get` and `prop_get` object macros. It assigns the documented base
interest for acknowledged props, enemies, corpses, and enemy orphans; removes
self-vehicle interest; adds movement, vehicle, and shooting weight; applies
the quantized-distance falloff; and returns the accumulated `real`.

Compile-time layout proofs bind the source fields to January: AI debug print
byte `+0x9D`, actor vehicle handle `+0x158`, secondary-look type/priority/timer
`+0x544/+0x546/+0x548`, and the relevant prop fields from `+0x18` through
`+0x12F`. `ai_debug.h` defines `__unknown3C` as a 105-byte character array, so
the typed index 97 access and its `+0x9D` proof are consistent. No protected AI
source or header is changed.

The stop body has `DIR32`/`REL32` references to `_actor_data`, `_datum_get`,
`_ai_debug+157`, `_temporary`, `_ai_debug_describe_actor`, the owned diagnostic
string, and `_console_printf`. The interest body has typed actor/prop data and
datum-get references plus its float references. All 24 relocations match
January symbolically, including their addends.

## Data and ownership boundary

Two target-owned COMDATs used by the retained code are independently exact:

| Owner | Bytes | Relocations | Payload SHA-256 |
| --- | ---: | ---: | --- |
| `??_C@_0O@FNONJPCB@?$CFs?3?5look?9stop?$AA@` | 14 | 0 | `3eb233ae5493c0a59f2bc647e6c879e17baa3497b1897d6b0cdaad82a3e89599` |
| `__real@3fe66666` (`1.8f`) | 4 | 0 | `bbe1d6b54f890c9431839072d39fcb11e98717384223934286b95f5c23a7dfa8` |

The candidate also emits eight ordinary SELECT_ANY float COMDATs referenced by
the exact interest code. Their corresponding target references are undefined
generic symbols in this split object, so those 32 candidate bytes are
explicitly uncredited as target-owned data. Conversely, the target's
anonymous 112-byte priority/time tables are absent from the candidate and
remain wholly unclaimed. The candidate owns no `.data`, `.bss`, COMMON symbol,
or other writable storage. The target's 1,700 aggregate data bytes therefore
remain outside campaign data credit for this partial unit.

## Policy, scope, and validation

Only `source/ai/actor_looking.c` and this new Jonas-owned ledger change. No
header, configuration, semantic exception, parked record, status/completion
label, pre-existing Markdown, Claude-owned file, or protected/frozen source is
edited, and no tracked path is deleted. The implementation is readable typed
C with one parameter per line and an explicit terminal return in the void
function.

The retained source contains no assembly, raw address or byte-offset access,
pointer/integer reconstruction, cast or union pun, inactive-union access,
undefined signed overflow, `volatile` scheduling device, force-inline
annotation, optimizer pragma/barrier, synthetic anchor, or object-byte/code-
generation forcing.

- Complete `halobetacache_build` and `libcmt_build`: passed (569/569 actions).
- Direct hardened comparison: both retained bodies passed; all other target
  functions remained absent from the candidate.
- Semantic audit: 470 units, 4,084 functions evaluated, 3,944 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,005 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,992/11,060 exact functions,
  477,671/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave added two functions and
  406 meaningful code bytes with no campaign data increase.
- Halo progress: 273/468 complete objects, 3,825/7,574 exact functions,
  464,757/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remained.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests passed; `git diff --check`, deleted-
  path, protected-scope, and banned-policy scans were clean.

After the source and ledger were committed, a clean committed-state regression
snapshot/check passed. The built `actor_looking.obj` was then deleted and
forced through the normal same-path Ninja edge; the post-rebuild regression
check and direct hardened comparison again passed both functions, reproducing
their padded sizes, normalized hashes, 24 relocations, and non-code ownership
boundary. The raw object-container SHA is deliberately not a reproducibility
criterion because VC7 writes the compile time into the COFF header. The
committed source blob and stable payload SHA above were reproduced from `HEAD`.
The final worktree was clean. No push was performed.
