# `actors.obj` Jonas maintenance pair

## Result

This bounded source-only wave starts from authoritative integration commit
`5c14f9595e860b598a2dfc49631cc6f3c4534664`. Three public actor-maintenance
bodies were emitted together in one natural XDK candidate object. Two were
independently strict and are retained. The nonexact position-sampling body and
all of its support-only layout checks were removed immediately, without a
spelling, declaration, control-flow, or compiler-control retry.

`actors.obj` advances from 21/76 to 23/76 exact functions and remains
`NonMatching`. The wave adds 174 meaningful and 192 padded code bytes with 11
relocations. It adds no target-owned data.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_change_encounter` | 107 | 112 | 7 | `d786fc03d079deeb4250f44f6a410d82a05418342e862a34ce6a3e199ed4450a` |
| `_actor_delete_props` | 67 | 80 | 4 | `dacd78ce17bd6fb2a750ad6c201dab343a4ed77c91a4a6dbdd6580cc5e5d11a5` |
| **Wave gain** | **174** | **192** | **11** | |

The hardened COFF comparator proves identical padding, normalized bytes, and
every relocation address, type, symbolic destination, and addend for both
retained bodies. The final object reports 1,448/19,130 meaningful code bytes
and 23 exact functions. The other 53 functions and all 2,288 target data bytes
remain outside this wave's credit.

## Provenance and signatures

- Baseline `source/ai/actors.c` Git blob:
  `e4cca3da3a422a9c171fa00bb15162724d3ef085`.
- Retained source Git blob / stable committed-payload SHA-256:
  `e08b43bafa362cc567101cfef4fe6a62ff21a0b1` /
  `2f350fee84d2e0754bc53bedd33de052e98d1d7b45a516f3eaa8850d77b9a678`.
  The SHA is over `git cat-file blob` payload bytes, not checkout EOLs.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `actors.obj` SHA-256:
  `632a2b344e779e5d58e4d9e8e881700d8d106d51fdc3b74cc731aff9e7c3d9b9`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name body blobs are
  `add38d70cda000cde4e6fcce9959619788002af3` for prop deletion,
  `b5d7245bfa2b962a5112430ccfb2e7c5dc920ec5` for encounter change, and
  `196926a146796712cd14ce78874ca8173f77d947` for position sampling. HCEA
  establishes the public meanings and natural typed operations; January's
  PDB, split object, and PC codeflow remain authoritative for the signatures,
  exact extent, call graph, bytes, padding, and relocation ownership.
- The independently preserved readable January-PC reconstruction has
  `actors.c` blob `9fc61cae8c2f5295c829e4cc1cbf6a986179c8d2`; it corroborates the
  first-prop deletion loop and its `actor_switch_props` then `prop_delete`
  ordering.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

January's PDB and split object authenticate these ordinary cdecl signatures:

```c
void actor_change_encounter(
    long actor_index,
    long encounter_index,
    short squad_index);

void actor_delete_props(
    long actor_index);
```

## Authenticated typed behavior

`actor_change_encounter` resolves the actor through the existing typed
`actor_get` macro and calls the already-exact `actor_flush_position_indices`.
It detaches from the encounterless list or current encounter as appropriate,
then attaches to the requested encounter and squad, or to the encounterless
list when the requested handle is `NONE`. Its four external helpers use
ordinary TU-local typed declarations.

`actor_delete_props` resolves the actor through `actor_get` and loops on the
named `meta.first_prop_index` field. Each iteration first calls
`actor_switch_props(actor_index, first_prop_index, NONE)`, then calls
`prop_delete` with the actor's newly current first prop. Re-reading the list
head after each helper reproduces January's loop and allows `prop_delete` to
re-head the list naturally.

Source-local layout proofs bind `meta.encounterless`, `meta.encounter_index`,
and `meta.first_prop_index` to January offsets `+0x09`, `+0x34`, and `+0x50`.
Both functions use one parameter per line and end with an explicit terminal
`return;`.

## One-shot rejection record

The first Ninja invocation stopped before launching the compiler because the
fresh worktree did not yet have its untracked XDK junction. It produced no
object and exercised no candidate code. The standard toolchain junction was
then restored without changing source, and the next invocation emitted the
sole three-body candidate object.

| Rejected function | Target meaningful / padded / relocs | Target hash | Sole natural candidate |
| --- | ---: | --- | --- |
| `_actor_input_sample_position` | `107 / 112 / 6` | `eaabaf06260c93ff3c0291fc228ffa7680687ae1a8e706ac64d6f246acb6e55a` | Rejected at 113 meaningful / 128 padded / 6 relocations, SHA `71a53d044a23917f2aa33bbc129c33be52cbba6e44b5d699db594921e5ed3acc`. The first four relocation addresses match January, but the ultimate-parent and final typed-object lookup move from target `+75,+83` to candidate `+80,+89`; the natural typed expression triggers an earlier stack cleanup and a different call schedule. |

The rejected COMDAT and all six of its support-only position/unit/object layout
checks are absent from the final object. Reopen it only with independently
preserved January source or a new authenticated calling-expression contract;
do not retry expression grouping or emulate the stack schedule.

## Whole-TU, data, and symbol boundary

The clean pre-wave regression snapshot was taken at exact commit `5c14f959`.
Its fail-closed comparison identifies exactly the two retained functions as
`NEWLY_EXACT`, reports `changed_nonexact: []`, and finds no runtime non-code
change. The only non-code difference is the ordinary `.debug$S` update.

Fourteen inherited accepted functions remain byte-for-byte stable in the
manifest. Seven later unit-control COMDATs retain identical padded sizes,
normalized hashes, relocation addresses/destinations, flags, selection, and
public ownership under the direct comparator; their manifest fingerprints
change only because inserting `actor_change_encounter` before them advances
their numeric COFF section indices. The direct hardened comparison passes all
21 inherited functions together with the two new bodies.

The symbol-set growth is the expected natural shape: public owners and `.text`
section symbols for the two new COMDATs plus their ordinary undefined helper
references. No existing owner changes semantically. The final source adds no
literal or initialized storage. The baseline `.rdata`, `.data`, and `.bss`
ownership and payloads remain unchanged, so this wave claims zero data bytes.

## Policy, gates, and committed replay

Only `source/ai/actors.c` and this new Jonas-owned ledger change. No shared
header, configuration, semantic exception, parked record, completion label,
pre-existing Markdown, Claude-owned file, protected-five source, or runtime
storage is edited.

The retained source is readable typed C. It contains no assembly, raw address
or byte-offset dereference, pointer/integer reconstruction, pointer or union
pun, inactive-union access, undefined signed overflow, volatile scheduling
device, force-inline annotation, optimizer pragma/barrier, synthetic anchor,
or object-byte/code-generation forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass (575 actions).
- Direct hardened comparison: all 23 inherited-plus-new strict functions pass;
  the rejected position-sampling symbol is absent.
- Semantic audit: 470 units, 4,090 functions evaluated, 3,950 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,011 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,998/11,060 exact functions,
  478,095/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds two functions and 174
  meaningful code bytes with no data increase.
- Halo progress: 273/468 complete objects, 3,831/7,574 exact functions,
  465,181/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check`, protected-
  scope, deleted-path, and banned-policy checks are clean.

After the source and ledger were committed, a clean committed-state
regression snapshot/check passed. The built `actors.obj` was then deleted and
forced through the normal same-path Ninja edge; the post-rebuild regression
check and direct hardened comparison again passed the 23 strict functions,
reproducing both new hashes, all 11 new relocations, and the unchanged runtime
non-code boundary. The raw object-container SHA is not a reproducibility
criterion because VC7 writes the compile time into its COFF header. The
committed source blob and stable payload SHA above were reproduced from
`HEAD`. The final worktree was clean. No push was performed.
