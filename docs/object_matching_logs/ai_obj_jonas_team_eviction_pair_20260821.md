# `ai.obj` Jonas team-status / vehicle-eviction pair recovery

## Result

This bounded source wave starts from canonical commit
`6de91d1d95a71515d03cac7785cda00175c0233c` and recovers both selected
public AI functions on the first natural two-body candidate compile. The nine
pre-existing exact functions remain strict, so `source/ai/ai.obj` advances
from 9/46 to 11/46 exact functions and remains `NonMatching`.

| Exact function | RVA | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- |
| `_ai_try_vehicle_eviction` | `0x2E3D0` | 109 | 112 | 5 | `69ee26606e0194ecd45524ccf956c6637eda50325d85df985c82c1104d75c001` |
| `_ai_update_team_status` | `0x2E890` | 218 | 224 | 11 | `1ed8268189619b90e7e389fe308750789431b5334d99c99a6c193e13c012fadb` |
| **Wave gain** | | **327** | **336** | **16** | |

The retained eleven-function subset is 748 meaningful bytes, 832 padded
bytes, and 52 relocations. Hardened comparison reports exact normalized
bytes, padding, relocation types and order, destinations, and addends for
both new functions and all nine inherited exact functions.

## Provenance and signatures

- The canonical baseline `source/ai/ai.c` blob is
  `6b5ec508389c499d940cf24e8fe9ba43edfe3f19`; the baseline
  `source/game/players.c` blob is
  `03f0be5922f756eb1bfb1e3507327f4b639a354f`. The retained committed
  payload identities are recorded in the additive replay evidence after the
  source-bearing commit.
- The January executable SHA-256 is
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- The January split `source/ai/ai.obj` is 42,421 bytes with SHA-256
  `5b207c27308c3e78b7adf5bfc14f65c7cf8cc99aeba15b845270eb572265ced2`.
- The extracted January PDB-symbol JSON SHA-256 is
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
- The clean HCEA oracle is pinned at
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact function blobs are
  `541bd1c93f0bd5e61801a325fe8fb05495a8b608` (vehicle eviction) and
  `a7f0dac0ba8901832f6d5dcff4e3140ab68f9831` (team status).
- The clean Stian PC disassembly reconstruction is pinned at
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its `src/halo/ai/ai.c` blob is
  `e273c750eea58d0e43daf1eb0a9037685cf01094`. Its VC7.1 verification gives
  100% instruction and operand scores to the team-status body at
  `0x40280..0x4035A` and to the semantically identical, renamed
  `ai_handle_unit_approach` body at `0x3FDC0..0x3FE2D`. This is PC
  disassembly reconstruction / crossbuild corroboration, not authentic source
  authority; HCEA and the January COFF, disassembly, and callers remain the
  authorities.
- The compiler is Microsoft 32-bit C/C++ Optimizing Compiler
  `13.00.9254.1` for 80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the repository's
  unchanged XDK 3911 and include paths.

The January callers and clean HCEA declarations establish the public
signatures:

```c
boolean ai_try_vehicle_eviction(
	long actor_index,
	long entering_unit_index,
	boolean immediate);
void ai_update_team_status(void);
```

The C identifiers map to the January external COFF symbols
`_ai_try_vehicle_eviction` and `_ai_update_team_status`. Each target symbol is
a type-`0x20`, storage-class-2 definition and each callee ends in an ordinary
cdecl `ret`.

January has two eviction consumers. `_code_000ab440` in `players.obj`
relocates the call at section offset `+0x115`; immediately before the call it
pushes `1`, the entering unit, and the actor index, then reclaims 12 stack
bytes. `_unit_find_nearby_seat` in `units.obj` relocates its call at `+0x1C6`;
it pushes `0`, the entering unit, and the actor index, reclaims 12 bytes, and
tests `AL`. This proves three 32-bit cdecl arguments and a boolean return.

January has five zero-argument team-status consumers:
`_code_00044440`, `_ai_scripting_migrate_by_unit`, and
`_ai_scripting_set_team` in `ai_script.obj`, plus `_encounter_attach_unit` and
`_encounter_attach_actor` in `encounters.obj`. Their relocation offsets are
respectively `+0x538`, `+0xFE`, `+0x5B`, `+0xFC`, and `+0x197`; none pushes an
argument for this call.

## Authenticated control and layout

Vehicle eviction obtains the actor first through the canonical `actor_get`
macro, yielding the target's `_actor_data` DIR32 relocation and `_datum_get`
call. Unless the entering handle is `NONE`, it obtains that unit through
`unit_get`. It succeeds only for a player-owned entering unit whose owner team
is not hostile to the actor team, and emits
`actor_stimulus_vehicle_eviction(actor_index)` only when `immediate` is true.
Its five target relocations, in order, are `_actor_data`, `_datum_get`,
`_object_get_and_verify_type`, `_game_team_is_enemy`, and
`_actor_stimulus_vehicle_eviction`.

Team-status update creates the public active-only actor iterator, creates one
prop iterator per actor, obtains every prop unit, and refreshes the typed
`team_index`, `enemy`, `ally`, `unopposable_enemy`, and `target_weight`
fields. Its eleven target calls, in order, are `_actor_iterator_new`,
`_actor_iterator_next`, `_prop_iterator_new`, `_prop_iterator_next`,
`_object_get_and_verify_type`, `_game_team_is_enemy`, `_game_team_is_ally`,
`_actor_compute_prop_unopposable`, `_actor_compute_prop_target_weight`,
`_prop_iterator_next`, and `_actor_iterator_next`.

The TU-local `actor_iterator` is a readable typed copy of the already
committed layout in `source/ai/encounters.c` blob
`5335791d253c6538c0e7d4b624c06726d11033f9`, not a cast or byte overlay. The
compile-time checks prove its `0x1C` size and `index` at `+0x14`. The accessed
production layouts are owned by `source/ai/actors.h` blob
`e061b529b05c782b1220cff8a05dd89e6e2f3ed2`, `source/ai/props.h` blob
`7b6cf11e7f9d04595564ee37e48e7efe582d7643`, and `source/units/units.h`
blob `54492a4ee4cb8b37739f9ebbfa89c72fbd0e08fc`.

The typed assertions prove actor team `+0x3E`, unit owner team `+0x68`, unit
player index `+0x1C8`, prop iterator size `0x8` and index `+0x0`, and prop
team/unit/target-weight/enemy/ally/unopposable fields at
`+0x12/+0x18/+0x50/+0x60/+0x61/+0xA4`. Existing committed implementations
independently establish the called helper topology:
`source/game/game_allegiance.c` blob
`e18fd7a9451d8b5f12815e7864b5d5812554cfaa`,
`source/ai/actor_stimulus.c` blob
`82a3e128abbbaa536bdc1134310acd383f609cda`, `source/ai/props.c` blob
`c2c0338e27830607733c7bea6315eafcde294d4e`, and `source/ai/actors.c` blob
`e08b43bafa362cc567101cfef4fe6a62ff21a0b1`.

## Bounded compile and storage boundary

Both selected `ai.c` definitions were emitted together in exactly one natural
candidate compile. Each body was independently strict on that first shot, so
no declaration, expression, or control shape was tuned and no retry occurred.
Parse-only prerequisite checks were transparent. Later clean forced rebuilds
are reproducibility replays of the accepted source, not candidate searches.

The only additional production correction is the local forward declaration
of `ai_try_vehicle_eviction` in `source/game/players.c`: its stale `void`
return is corrected to the caller-proven `boolean`. A controlled object A/B
against exact commit `6de91d1d` compares the 44,752-byte baseline
`players.obj` (SHA-256
`db40d00ede34a7a82cacd8fd8f2fb51b8afaac0bfccec0185cb120a9513a650b`)
with the 44,753-byte corrected object (SHA-256
`01c05712ce7ea2b5b7b7820214e05654d5ca070f4ff37496a39f0634c13acb9f`).

All 82/82 runtime code sections have identical normalized bytes, padding,
relocation counts, types, order, destinations, and addends. All 39/39 runtime
non-code sections and the one directive section are identical. The 227/227
external symbol records, including their owner identities, values, types, and
storage classes, are identical with no addition or removal. Eighteen of 19
debug sections are identical. Only the relocation-free CodeView
`.debug$S|anonymous=0` record changes from 186 bytes / normalized SHA-256
`11ae6d9e94dd7953a0d8211fde5cfd0a634b8273746b1a5c3a757db33c96d9fc`
to 187 bytes / normalized SHA-256
`7475619da8d7413833d295edf850c584d2dce0ed7683783ca8677501c722365d`.
Compiler-local `$L...` names in `_code_000ab440` and `_code_000ace70` are
renumbered by that CodeView type correction; hardened internal-destination
comparison proves both runtime sections strict. There is no runtime drift.

Neither recovered AI body owns or references a string, literal pool, global
definition, `.data`, `.bss`, or other non-code section. All 16 relocations are
to undefined external helpers or data owned elsewhere. `ai.obj` therefore
advances to 748/12,932 meaningful code bytes while claiming 0/3,324 target
non-code bytes.

## Policy and scope

Only `source/ai/ai.c`, the local declaration correction in
`source/game/players.c`, and this new Jonas ledger are changed. No shared
header, configuration, protected AI source, semantic exception, parked
record, completion label, pre-existing Markdown, or storage definition is
modified. The other 35 unmatched `ai.c` bodies remain omitted.

The implementation uses typed public helpers, named fields, ordinary cdecl
functions, and readable control flow. It contains no assembly, volatile
scheduling, forced inline, optimizer pragma or barrier, raw address, pointer
arithmetic, type pun, unaligned load, inactive-union read, undefined overflow,
synthetic anchor, or object-byte forcing.

## Candidate-stage regression and validation

- Complete Halo and libcmt builds plus ordinary and semantic reports: pass.
- Semantic audit: 470 units, 4,133 functions evaluated, 3,993 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,054 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,041/11,060 exact functions,
  481,258/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. Halo contributes 3,874/7,574
  functions, 468,344/1,770,166 code bytes, and
  1,829,896/3,923,451 data bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- The pre-wave whole-TU gate identifies exactly the selected pair as
  `NEWLY_EXACT` and reports `changed_nonexact: []` for both `ai.obj` and
  `players.obj`. Adding two AI COMDATs shifts later compiler-local symbols and
  debug metadata; direct hardened comparison independently proves all eleven
  accepted AI functions strict. The controlled `players.obj` A/B above
  independently proves every runtime section and all external ownership
  unchanged. No adjudication or configuration exception is added.
- `git diff --check`, deleted-path, protected-scope, and changed-line
  banned-construct checks pass.

No push is performed.
