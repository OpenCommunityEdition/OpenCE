# `ai.obj` Jonas six-leaf recovery

## Result

This bounded source-only wave starts from authoritative integration commit
`3d70e392822ef5e51d1caf894a67590d5cab398d` and recovers all six reviewed AI
leaves on the first natural candidate compile. The pre-existing exact
`_ai_place` body remains byte-for-byte strict. `ai.obj` therefore advances
from 1/46 to 7/46 strict functions and remains `NonMatching`.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_ai_dispose_from_old_map` | 50 | 64 | 9 | `e429963c353e8ed2e6a5cae663f32ef0dc751629bd5d6d1df6abe74a66363a6a` |
| `_ai_globals_ai_active` | 64 | 64 | 6 | `498831ae651b491ce0a0a61aeda25808fd50bdad26c4f2389c1928b15d8ada82` |
| `_ai_globals_dialogue_triggers_enabled` | 66 | 80 | 6 | `54336053582728f8853fa023e3b25d12beac2f491708a673f616c37e20eaa6c0` |
| `_ai_globals_grenades_enabled` | 72 | 80 | 6 | `a6a8ab146a9f0e21c92c347e5718293ca7820210a2b66b14103e35181aaf8d68` |
| `_ai_enemies_can_see_player` | 11 | 16 | 1 | `656e3364cc241ffc681ead3c977f738c38ceb73a83dd22b925ea6d58fe5dd3ba` |
| `_ai_enemies_attacking_player` | 11 | 16 | 1 | `7b6f2b3b6b931f38f38651c11c4bef318c1003acdaf4a275be599bc8715c5255` |
| **Wave gain** | **274** | **320** | **29** | |

The retained seven-function subset, including `_ai_place`, is 279 meaningful
bytes, 336 padded bytes, and 30 relocations. The hardened COFF comparator
reports `all_equal: true` for all seven functions, including normalized bytes,
padding, relocation types and order, destinations, and addends.

## Provenance and signatures

- The baseline `source/ai/ai.c` blob is
  `117311ec888cb2f7c66c4050bb70d682cc9eb16e`.
- The January executable SHA-256 is
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- The January split `ai.obj` SHA-256 is
  `5b207c27308c3e78b7adf5bfc14f65c7cf8cc99aeba15b845270eb572265ced2`.
- The extracted January PDB-symbol JSON SHA-256 is
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
  It independently fixes the six public symbol names and RVAs.
- The clean HCEA source oracle is pinned at
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`. The relevant blobs are
  `0d7ed6b4e14f7c44da1e954c43b6e8b03abbdc61` (dispose),
  `ff6d2be92082f9ec1adf630703e86e72525b13b8` (active setter),
  `227e6797bcf6a43c6b1f229857caff7bbc321714` (dialogue setter),
  `0d0e914a9d45c79dd65decd361271d97d96e7ef5` (grenade setter),
  `2ec6cb8e73d9447bfd5910fc0d11bd7fd80df14c` (visibility wrapper),
  `32f049cf3a6c7cf2de1bf22a1142bd03952f2c2c` (attack wrapper), and
  `c5c3693f1d4cef82db9b2aa91ad2bc90471a6af1` (`ai_globals.h`).
- The preserved January-PC source oracle is clean at
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its `ai.c` blob is
  `e273c750eea58d0e43daf1eb0a9037685cf01094`.
- The compiler is Microsoft 32-bit C/C++ Optimizing Compiler
  `13.00.9254.1` for 80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Generated flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 and include paths.

Repository callers and HCEA establish these exact public signatures:

```c
void ai_dispose_from_old_map(void);
void ai_globals_ai_active(boolean enabled);
void ai_globals_dialogue_triggers_enabled(boolean enabled);
void ai_globals_grenades_enabled(boolean enabled);
boolean ai_enemies_can_see_player(void);
boolean ai_enemies_attacking_player(void);
```

The two wrappers return a Boolean result; the current `players.c` consumes the
attack predicate in a conditional. Their shared typed internal callee takes a
Boolean selecting visibility-only (`FALSE`) or actively-attacking (`TRUE`).

## Authenticated control and layout

HCEA supplies the lifecycle and wrapper intent. January source and COFF fix
the target-specific details which HCEA does not preserve: dispose calls AI
communication, AI script, encounters, props, actors, paths, AI profile, and
AI debug in that order before clearing `ai_initialized_for_map`. The target's
nine relocations occur at `+1`, `+6`, `+11`, `+16`, `+21`, `+26`, `+31`,
`+36`, and `+41` and resolve to those eight typed functions followed by
`_ai_globals`.

The three setters preserve the January assertions on `ai_globals` at original
source lines `0x13A`, `0x143`, and `0x14C`, then perform one byte-sized field
store. HCEA's full globals declaration and January accesses independently
authenticate active at `+0x00`, initialized-for-map at `+0x01`, dialogue at
`+0x10`, and grenades at `+0x3B4`.

The source declares only a natural prefix through the last accessed field and
proves all four offsets plus prefix size `0x3B5` at compile time. `_ai_globals`
is an ordinary typed external pointer. In both the target comparison boundary
and the retained candidate it is referenced symbolically; this translation
unit defines no backing storage, COMMON owner, `.bss`, or `.data` for it.

## Bounded compile and data boundary

All six definitions were emitted together in exactly one natural candidate
compile. Every body was independently strict, so no body, declaration,
expression, or control shape was changed and no retry was performed. A later
clean forced rebuild is only a reproducibility replay of this accepted source.

The translation unit now measures 7/46 strict functions and 279/12,932
meaningful code bytes. It claims 0/3,324 target non-code bytes. The assertion
source and expression strings are authentic dependencies of the exact setter
code, but this incomplete object receives no independent data credit.

## Policy and scope

Only `source/ai/ai.c` and this new Jonas-owned ledger are changed. No shared
header, configuration, frozen source, semantic exception, parked record,
pre-existing Markdown, or Claude-owned file is modified. `_ai_place` is
preserved and `ai_initialize` plus every unreviewed function remain omitted.

The retained code uses typed prototypes, named fields, ordinary Boolean values,
and target-authentic assertions. It contains no assembly, volatile scheduling,
forced inline, optimizer pragma/barrier, raw address, pointer arithmetic,
type pun, inactive-union read, undefined overflow, synthetic anchor, or object-
byte forcing.

## Regression and validation

The clean pre-wave translation-unit manifest identifies exactly the six
reviewed functions as `NEWLY_EXACT` and reports `changed_nonexact: []`. Its
generic structural comparison also reports the expected compiler debug,
string-COMDAT, symbol-inventory, and section-index changes caused by inserting
new COMDATs. In particular, `_ai_place` shifts from candidate section 3 to
section 4 while retaining the same 16-byte padded hash and sole relocation;
direct strict comparison proves it did not regress. No adjudication or
configuration exception is added.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Strict semantic audit: 470 units, 4,009 functions evaluated, 3,869 semantic
  exact, 101 hidden exact / 61,876 hidden code bytes, 3,930 accepted exact,
  and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,917/11,060 functions,
  473,628/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 data bytes.
  Halo contributes 3,750/7,574 functions and 460,714/1,770,166 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; JSON and diff checks are clean.
- A clean committed-state snapshot/check and deletion-plus-forced rebuild of
  `ai.obj` reproduce the accepted object and pass the same strict comparison.

No push is performed.
