# `hud_messaging.obj` Claude exact trio and owner recovery (2026-08-31)

## Result

Starting from canonical commit `1b49fbacb`, this surgical integration closes
the three residual HUD-script leaves recovered in Fable commit `c8ff585ee` and
restores six authenticated private owners from Wave 4 commit `21f4fc550`.
The older donor file was not copied wholesale; canonical's newer exact HUD
bodies remain unchanged.

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_scripted_hud_set_flashing_state` | `61 / 64` | `3` | `73fad48b44459427137dfa32986983d66a6974088b495b4c9bc170e63b90caeb` |
| `_scripted_hud_set_timer_position` | `82 / 96` | `1` | `a7a7c828c68c5a0fdca09ccd7105727f171ee82300f026bbdc2d57c351b232cb` |
| `_scripted_hud_messages_clear` | `54 / 64` | `1` | `bf66bd5b02649e390b947906d4c174c568ee8e8e3df02662e87d1515b52a22b1` |

`hud_messaging.obj` advances from **19 exact / 3 residual / 14 unwritten**
to **22 exact / 0 residual / 14 unwritten**, adding 197 meaningful and 224
padded code bytes.  The recovered static `_hud_messaging_globals` pointer also
matches the target's four-byte, relocation-free BSS owner with normalized
SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.

The whole-board replay gains exactly those three functions, 197 meaningful
code bytes, and four data bytes, with no exact-function regression.

## Source and ownership

The retained source changes are ordinary and semantic:

- `_scripted_hud_set_flashing_state` writes through the real
  `hud_messaging_globals` owner after sampling `game_time_get`, without the
  donor's unnecessary pointer alias;
- `_scripted_hud_set_timer_position` uses the canonical `PIN(corner, 0, 4)`
  macro instead of hand-written clamp logic; and
- `_scripted_hud_messages_clear` keeps its existing natural datum loop; the
  corrected static owner supplies its exact relocation identity.

The private global changes from an address placeholder declared `extern` to
its HCEA-recovered name and target-proven storage:

```c
static struct hud_messaging_globals_definition *hud_messaging_globals;
```

This is a real zero-initialized TU owner, not a linkage trick or a fabricated
global.  All same-TU references use the semantic name.

The following `symbols.json` records are restored and marked target-static:

| File offset | Old owner | Recovered owner |
|---:|---|---|
| `800784` | `_code_000c3810` | `_render_state_text` |
| `800912` | `_code_000c3890` | `_render_state_bitmap` |
| `803824` | `_code_000c43f0` | `_find_free_hud_message` |
| `803952` | `_code_000c4470` | `_compare_messages` |
| `3034188` | `_data_002e4c4c` | `_button_mappings` |
| `4537016` | `_bss_00453ab8` | `_hud_messaging_globals` |

January's exact symbol atlas independently names `render_state_text`,
`render_state_bitmap`, and `find_free_hud_message`.  HCEA PDB/source records
corroborate the other function, data, and BSS owners and their private scope.
The first four functions remain unwritten; their changes are semantic names,
not code credit.

HCEA's later Xbox `_button_mappings` payload is only 14 bytes while January's
PC target owns 28 bytes.  This packet transfers the authenticated name and
scope only; it does not copy or synthesize HCEA data bytes.

## Prototype ownership and ABI

The public prototypes for `scripted_hud_set_flashing_state` and
`scripted_hud_set_timer_position` now live in the owner
`interface/hud_messaging.h`, alongside the already-owned clear prototype.
`hs.c` includes that header and no longer declares any of the three locally.
This also corrects the timer-position ABI from the misplaced unsigned `word`
form to the source-authenticated signed `short` parameters.

The owner header had four existing C consumers (`players.c`, `hud.c`,
`hud_messaging.c`, and `saved games/game_state.c`); `hs.c` becomes the fifth.
The declaration-count change was treated as a hard codegen hazard.  PID-local
gates and the full production rebuild retain:

- `hs.obj`: 445 exact / 3 residual / 0 unwritten;
- `players.obj`: 52 / 17 / 1;
- `hud.obj`: 10 / 0 / 12;
- `units.obj`: 189 / 0 / 0; and
- all pre-existing exact sections in `game_state.obj` and every other unit.

No dummy declaration or misplaced fallback prototype is used.

## Provenance and policy

- Byte-producing donor: `c8ff585ee941bd8454e991d7512b5b36ba0f161a`.
- Name/static donor: `21f4fc5505754272da1ba64b7f0d51b08c6517d7`.
- HCEA source/PDB donors: the matching `render_state_*`,
  `find_free_hud_message`, `compare_messages`, scripted-HUD, button-map, and
  HUD-global records in the pinned local HCEA research trees.
- January split target and regenerated `symbols.json` remain authoritative for
  bytes, relocation identity, size, and storage class.

The retained C contains no address-named owner, raw offset, representation
cast, assembly, pragma, forced inline, volatile/barrier, synthetic anchor,
fixed/dead condition, or nonsensical write.  It uses canonical engine types
and `PIN` rather than manual bit or clamp logic.

## Validation

- configured production split regenerated with the six recovered owners;
- `tools/campaign/gate.py source/interface/hud_messaging --all`:
  `22 exact / 0 residual / 14 unwritten`;
- strict board: `1248 / 6704` meaningful bytes and `22 / 36` functions;
- rename-aware whole-tree verdict diff: exactly three `R -> E` gains and zero
  regressions;
- full configure/Ninja build, ordinary progress, and semantic progress: pass;
- semantic audit: 470 units, 5,027 functions evaluated, 4,919 semantic exact,
  4,929 accepted exact, zero unit errors;
- campaign total: 628,102 / 2,198,102 meaningful code bytes and
  4,886 / 11,060 accepted functions;
- Halo total: 615,188 meaningful code bytes and 4,719 functions;
- exact data total increases by four bytes to 1,866,542;
- `units.obj` remains 189/189.

No semantic exception, parked record, generated report, completion label, or
HCEA data payload is added.  Commit and push remain integration-owner actions.
