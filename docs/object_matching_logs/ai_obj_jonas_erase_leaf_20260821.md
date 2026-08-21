# `ai.obj` Jonas erase-leaf recovery

## Result

This bounded wave starts from exact canonical commit
`63c0252e3d088d52daaa59f45d2320555dac790f`. The authorized natural
two-body candidate compile tested `ai_adjust_damage` and `ai_erase` together.
`ai_erase` was independently strict on that first emission and is retained;
`ai_adjust_damage` missed and was removed with every support-only declaration
without tuning or retry. The eleven pre-existing exact `ai.obj` functions
remain strict, so the unit advances from 11/46 to 12/46 exact functions and
remains `NonMatching`.

| Retained function | RVA | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- |
| `_ai_erase` | `0x2DF80` | 195 | 208 | 9 | `717db576dedb590f107df380df691b64a0d6177a5578d1f65a9184d5b485aa11` |

The retained twelve-function subset is 943 meaningful bytes, 1,040 padded
bytes, and 61 relocations. Direct hardened comparison reports exact normalized
bytes, padding, relocation types and order, destinations, and addends for the
new function and all eleven inherited exact functions.

## Provenance and signature

- The baseline `source/ai/ai.c` Git blob is
  `f5a5de883e41c754802eaaa67a41c403fa0eb66a`. The retained committed payload
  identity is recorded in the additive replay evidence after the
  source-bearing commit.
- The January executable SHA-256 is
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- The January split `source/ai/ai.obj` is 42,421 bytes with SHA-256
  `5b207c27308c3e78b7adf5bfc14f65c7cf8cc99aeba15b845270eb572265ced2`.
- The extracted January PDB-symbol JSON SHA-256 is
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
- The clean HCEA semantic donor is pinned at
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact function blob for
  `ai_erase` is `1c01d6cb68232d06839e4f1abfc60563e1976ec3`.
- The clean Stian PC disassembly reconstruction is pinned at
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its `src/halo/ai/ai.c` blob is
  `e273c750eea58d0e43daf1eb0a9037685cf01094`. It is crossbuild
  corroboration only, not authentic source authority. HCEA and the January
  COFF, disassembly, and callers remain authoritative.
- The compiler is Microsoft 32-bit C/C++ Optimizing Compiler
  `13.00.9254.1` for 80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the repository's
  unchanged XDK 3911 and include paths.

The January callers and clean HCEA declaration establish the public
signature:

```c
void ai_erase(
	long encounter_index,
	long platoon_index,
	long squad_index,
	boolean immediate);
```

The C identifier maps to the January external COFF symbol `_ai_erase`, a
type-`0x20`, storage-class-2 definition ending in an ordinary cdecl `ret`.
Three independent January call shapes agree. `ai_scripting_erase` pushes
`FALSE`, squad, platoon, and encounter; `ai_scripting_erase_all` pushes
`FALSE`, `NONE`, `NONE`, and `NONE`; and `ai_release_inactive_encounters`
pushes `TRUE`, `NONE`, `NONE`, and the encounter. Each caller reclaims 16
stack bytes. This proves four 32-bit cdecl arguments and the void return.

## Authenticated control and layout

The retained body first reads the existing typed
`ai_globals->ai_initialized_for_map` byte and returns when AI is unavailable.
For an all-encounters request, it creates the public actor iterator with
`active_only == FALSE`, advances it, and passes each typed iterator index plus
the caller's `immediate` value to `actor_erase`. For a selected encounter, it
creates the public encounter-actor iterator and erases only actors whose named
platoon and squad indices either match the requested values or are not
filtered because the corresponding argument is `NONE`.

The target's nine relocations occur in this exact order: `_ai_globals` DIR32
at `+7`, then REL32 references to `_actor_iterator_new` at `+38`,
`_actor_iterator_next` at `+47`, `_actor_erase` at `+71`,
`_actor_iterator_next` at `+80`, `_encounter_actor_iterator_new` at `+102`,
`_encounter_actor_iterator_next` at `+111`, `_actor_erase` at `+165`, and
`_encounter_actor_iterator_next` at `+177`.

The TU-local `encounter_actor_iterator` is a readable typed copy of the
already committed layout in `source/ai/encounters.c` blob
`5335791d253c6538c0e7d4b624c06726d11033f9`, not a cast, byte overlay, or
raw-offset access. Compile-time checks prove its `0xC` size and `index` at
`+0x4`. The existing TU-local actor iterator remains `0x1C` with `index` at
`+0x14`. The accessed production actor layout is owned by
`source/ai/actors.h` blob `e061b529b05c782b1220cff8a05dd89e6e2f3ed2`;
new typed checks prove `meta.squad_index` at `+0x3A` and
`meta.platoon_index` at `+0x3C`. The current helper implementations are
independently present in `source/ai/encounters.c` above and
`source/ai/actors.c` blob `e08b43bafa362cc567101cfef4fe6a62ff21a0b1`.

Clean HCEA independently supplies actor-layout blob
`77a69f79b6a42cf0dac4462e434e634ce71f7bc3`, actor-iterator blob
`679cb406c72b1b7f794d64ff37d8bfed89ee6797`, and encounter-actor-iterator
blob `3bdd8024ab3ff831f0e582a78071f9f6ee3c8ed9`. All reads in the production
body use named typed fields; no source spelling is inferred from offsets.

## One-shot adjudication

Both authorized definitions were emitted together in exactly one natural
candidate compile after transparent parse-only checks. `ai_erase` immediately
matched its 208-byte padded section, all nine relocations, and target hash.
The same candidate object preserved all eleven inherited exact functions and
had full-file SHA-256
`e57b4b0ff20bcdb2bf38b330628cce91c71ca74e80edca548cc815ce94b45308`.

`ai_adjust_damage` did not retain. Its January target at RVA `0x2DF10` is 105
meaningful / 112 padded bytes with four relocations and normalized SHA-256
`22cde8970724a5f0c0099a8025e7ebadedca3d09f2c395938ab80f7bde657184`.
The sole candidate also padded to 112 bytes with four relocations, but its
normalized SHA-256 was
`eef210bd0f2c455f786a2b2658a9a97cb3a7e313b0cd2596e1abe6a2e91ca133`.
Every relocation was uniformly five bytes late (`+21/+27/+57/+96` rather
than `+16/+22/+52/+91`). The body, `objects/damage.h` include, damage enum,
and all adjust-only layout checks were removed immediately. No expression,
declaration, branch, or source topology was tuned and there was no second
candidate emission.

Later retained-source builds and the clean committed forced rebuild are
verification replays of the independently accepted `ai_erase` source, not
candidate searches.

## Storage and policy boundary

`ai_erase` owns no string, literal pool, global definition, `.data`, `.bss`,
or other non-code payload. Its nine relocations target undefined external
helpers or the externally owned `_ai_globals` pointer. The wave therefore
adds 195/12,932 meaningful code bytes while claiming 0/3,324 target non-code
bytes.

Only `source/ai/ai.c` and this new Jonas-owned ledger change. No shared
header, configuration, protected AI source, semantic exception, parked
record, completion label, pre-existing Markdown, or storage definition is
modified. The other 34 unmatched `ai.c` bodies, including the removed
`ai_adjust_damage`, remain omitted.

The implementation uses typed public helpers, named fields, ordinary cdecl
code, and readable control flow. It contains no assembly, register ABI,
volatile scheduling, forced inline, optimizer pragma or barrier, raw address,
pointer arithmetic, cast overlay, type pun, unaligned load, inactive-union
read, undefined overflow, synthetic anchor, or object-byte forcing.

## Candidate-stage regression and validation

- Complete Halo and libcmt builds plus ordinary and semantic reports: pass.
- Semantic audit: 470 units, 4,141 functions evaluated, 4,001 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,062 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,049/11,060 exact functions,
  481,705/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. Halo contributes 3,882/7,574
  functions, 468,791/1,770,166 code bytes, and
  1,829,896/3,923,451 data bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- The retained `ai.obj` is 5,485 bytes with full-file SHA-256
  `db1a01eb3f963cbefc55e3c5c3aa6b64e44dd04858a04ab3b096e2c943a9b823`.
  Direct hardened comparison independently proves all 12/12 accepted bodies
  strict at 943 meaningful / 1,040 padded bytes and 61 relocations.
- `git diff --check`, changed-path, deleted-path, protected-scope, and
  changed-line banned-construct checks pass.

## Additive committed-payload and replay evidence

The source-bearing Jonas commit is
`938eabc26f95e8bd6072c484d3fff1b0de537897`. Reading the production source
payload directly back from that commit's Git tree authenticates
`source/ai/ai.c` as blob `0cac8b1e755a27bd3f32894904be1e437c4b9cb7`,
15,237 bytes, with payload SHA-256
`d57b67538b816c33b481c5f740565dbc03844c11e38503613606d2b40983ba7a`.

A clean one-unit regression snapshot was written at that exact commit. The
resolved `build/base/source/ai/ai.obj` path was explicitly verified to be
beneath the isolated worktree, the object was deleted, and Ninja rebuilt it
from the committed source with the unchanged production recipe.

The replayed `ai.obj` is 5,485 bytes with full-file SHA-256
`e55465fa65aabc6d1ee7d9043ecf6f518dbd00e4daedded3141b021a4b2f976d`.
Direct hardened comparison against the January object proves all 12/12
accepted bodies strict, including `_ai_erase` at the recorded 208-byte / nine
relocation shape and normalized hash. The clean snapshot check reports
`failures: []`, `warnings: []`, `changed_nonexact: []`, and all twelve
functions as `still_exact`; no body is newly exact or regressed during the
replay.

This replay evidence is added in a separate ledger-only commit. No amend,
history rewrite, or push is performed.
