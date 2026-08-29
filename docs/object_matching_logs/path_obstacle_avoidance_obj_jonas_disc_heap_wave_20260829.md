# `path_obstacle_avoidance.obj`: disc/heap typed wave (2026-08-29)

## Result

This bounded source-only batch starts from canonical commit
`9b116251eede8472c99fc2f4e4a4eefc4f2358ba`. Four adjacent ordinary-C
helpers are strict exact. `path_obstacle_avoidance.obj` advances from 5/24 to
9/24 strict functions, from 210 to 506 meaningful code bytes, and from 240 to
560 padded code bytes. It remains `NonMatching`; this batch claims no data
credit.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_obstacles_get_disc` | 78 | 80 | 4 | `25db6a5042f2dd9fca0dcf97ca4b293ef933ea993ef423078f0203fedd55e809` |
| `_obstacle_from_disc` | 36 | 48 | 1 | `eb1d71aa8cc2a13eceb4eff15d3668b4799a38ac3763a5fb3ecf8333b30da5b2` |
| `_path_get_step_index` | 94 | 96 | 4 | `9bc6568057ce4d851ae928a516dd583e0e47650281530c5906fb76b20d159e8d` |
| `_heap_cost` | 88 | 96 | 5 | `995a32819b2b4c4d4918c2e5ee0ee0746aa70e5fec519d8c2a51e829173f1f9e` |
| **Wave gain** | **296** | **320** | **14** | |

The hardened comparator reports `all_equal: true` for all four functions. It
verifies complete padded sections and all 14 relocation addresses, types,
targets, and addends. The five inherited exact functions remain exact; the
whole-object gate reports 9 exact, 0 residual, and 15 unwritten.

## Evidence and typed reconstruction

The target is the January `cachebeta.exe` build 2342 split object. Its
`path_obstacle_avoidance.obj` SHA-256 is
`773fcf3c4c50847f26051520cb99edd537ecbc8d810752be010d233acc009303`.
Compilation uses the pinned XDK 3911 VC7 compiler and the translation unit's
unchanged repository flags.

Both prior object ledgers were read in full. They document the existing
path-step, heap-index, and valid-point recoveries, but they neither attempt nor
reject these four functions. The target string synopsis independently supplies
the exact assertion file names, expressions, and source lines used here.

The user-supplied HaloCE archive at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` supplies independent semantic
sources for all four functions and typed `obstacles`, `disc`, and
`obstacle_path` records. January PC COFF remains authoritative where the later
build differs:

- HaloCE declares `obstacle_from_disc` as a 16-bit return and writes its
  `NONE` branch first. January sign-extends the disc field to a 32-bit `long`
  and uses the equivalent positive valid-index branch. The retained PC ABI and
  predicate reproduce both return paths exactly.
- HaloCE's disc record is `0x14` bytes. January's address calculation proves a
  `0x18` stride while preserving `obstacle_index` at `+2`; the source-local
  typed record therefore names a four-byte opaque tail and asserts the PC
  size, count, array offset, and field offset.
- HaloCE's accessors omit January's debug assertions. Target COFF and its owned
  strings prove the maximum-128 index checks, exact file literals, and source
  lines. The retained `match_assert` calls are natural diagnostics and match
  every relocation.
- `heap_cost` resolves the heap slot through the newly closed
  `path_get_step_index`, validates the resulting signed step index, and returns
  the typed `total_distance` field.

The first `obstacle_from_disc` emission followed HaloCE literally. Its equal
size and relocation set isolated the return-width and branch-topology ABI
differences; the two PC-authenticated corrections produced exact bytes. The
first `obstacles_get_disc` emission matched the entire assertion path and
differed only in the final `0x14` versus `0x18` element stride; the localized
PC layout correction made it exact. `path_get_step_index` and `heap_cost`
matched together on their first emission. No unrelated spelling, declaration,
scheduling, compiler-option, or brute-force matrix was attempted.

## Source policy and ownership

All accesses use named typed fields. Compile-time assertions prove the local
record sizes and offsets. The functions have one parameter per line and end
with explicit returns. The batch contains no assembly, intrinsic, barrier,
pragma, `volatile`, `register`, attribute, raw address or byte-offset access,
pointer/integer reconstruction, union or aliasing pun, undefined behavior,
synthetic anchor, compiler-flag change, object patch, comparator exception,
semantic allowlist, or parked entry.

Only `source/ai/path_obstacle_avoidance.c` and this additive ledger change. No
shared header, protected source, configuration file, global-storage owner, or
pre-existing ledger is modified. The assertion literals are natural compiler
output and receive no data credit.

## Validation

- Selected and whole-object campaign gates: all four new functions exact;
  9 exact, 0 residual, and 15 unwritten overall.
- Direct `tools.coff_compare`: `all_equal: true` for 320 padded bytes and all
  14 relocations.
- Full Halo and libcmt builds: pass.
- Progress: 575,689/2,198,102 meaningful code bytes and 4,593/11,060
  functions overall; Halo is 562,775/1,770,166 bytes and 4,426/7,574
  functions. This wave contributes exactly four functions and 296 meaningful
  bytes.
- Semantic audit: 470 units, 4,742 functions evaluated, 4,620 semantic exact,
  4,630 accepted exact, and zero unit errors or ordinary rejections.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 13 active, zero stale, and zero invalid; this wave adds no
  park.
- Tooling suite: 205/205 tests pass.
- `git diff --check`, deleted-path review, protected-source boundary, and
  retained-path review: pass.
