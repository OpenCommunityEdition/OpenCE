# GameState Xbox Opus remainder reconciliation

This packet reconciles the remainder of Opus commit `66f75b77b` against
canonical commit `7f7b72cc3819f05e3e85956101f314b544825e08`. It excludes the
already-admitted `game_state_allocate_buffer` owner from new credit.

## Disposition and exact accounting

The focused canonical gate begins at 6/17 exact functions, zero residuals and
11 unwritten functions. The candidate reaches **16/17 exact functions, one
credible residual and zero unwritten functions**. Relative to current
canonical, it adds **10 exact functions, 2,064 meaningful bytes and 2,144
padded bytes**:

| function | meaningful | padded |
|---|---:|---:|
| `game_state_get_persistent_storage_path` | 17 | 32 |
| `game_state_create_or_open_file` | 207 | 208 |
| `game_state_write_to_file` | 224 | 224 |
| `game_state_read_from_file` | 273 | 288 |
| `game_state_write_core` | 133 | 144 |
| `game_state_read_core_header` | 121 | 128 |
| `game_state_read_core` | 145 | 160 |
| `game_state_read_header_from_persistent_storage` | 422 | 432 |
| `game_state_write_to_persistent_storage` | 350 | 352 |
| `game_state_read_from_persistent_storage` | 172 | 176 |

The object would therefore own 2,606 meaningful / 2,720 padded exact code
bytes after admission. No whole-object `Matching` label and no data credit are
claimed.

All six inherited exact owners survive hardened code-and-relocation comparison:
`game_state_allocate_buffer`, `game_state_free_buffer`,
`game_state_close_file`, `game_state_get_persistent_storage_filename`,
`delete_persistent_storage`, and `game_state_create_persistent_storage`.

## Semantic names and ownership

The three January address placeholders are replaced together:

| placeholder | recovered private name |
|---|---|
| `_code_001b0220` | `game_state_get_persistent_storage_path` |
| `_code_001b0240` | `delete_persistent_storage` |
| `_code_001b0270` | `game_state_open_persistent_storage` |

The clean HCEA reference at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently identifies all
three names and their saved-games owner family. More importantly, the
cross-build correspondence atlas records all three as **static** members of
`saved games/game_state_xbox.obj`; that is stronger ownership evidence than a
name-only lift from a later build. `config/symbols.json` therefore records all
three with `"static": true`.

`game_state_get_persistent_storage_path` retains `static __inline` because the
January object itself proves that schedule: two callers contain the helper
body inline while an unreferenced, prologue-free out-of-line copy remains. A
non-static inline or ordinary function does not reproduce that combination.
This is a narrow target-proven exception to the general no-inlining rule, not a
forced optimizer trick. The focused guard confirms that it emits no
`point_from_line3d` COMDAT.

The global remains the existing semantic `xbox_game_state_globals`; no
`bss_004d27d0` source name or alias survives. Public prototypes remain in the
owning `saved games/game_state.h`. The player-profile call now comes from its
real `interface/player_ui.h` owner rather than a copied prototype. Xbox/Win32
SDK types, constants and imports come from the project-standard `<xtl.h>`
owner used by the neighboring saved-game files; no copied SDK declarations
remain in this translation unit. A focused alternate-source gate proved this
owner correction preserves all 16 exact functions, the single honest
residual and the forbidden-emission guard.

## Source authenticity

The bodies are constrained by January's complete callee census, diagnostic
strings and source-line anchors, plus per-function instruction and relocation
comparisons. The HCEA build independently corroborates the saved-game path
helper, open/create flow and private names. It also corroborates two odd but
target-authenticated behaviors in `game_state_open_persistent_storage`: the
profile path is fetched twice on the null-directory path, and failure cleanup
passes the bare profile path to `DeleteFileA`. These are preserved as original
behavior rather than normalized away.

No register keyword, volatile/barrier steering, pragma, assembly, raw-address
field access, fabricated dependency, undefined behavior or nonsensical
matching construct is introduced. Existing project types and macros are used;
function declarations and explicit returns follow the campaign house style.
The `0x345000` allocation extent is expressed with the established sibling
name `GAME_STATE_SIZE`; the January-proven page and persistent-file extents
remain named TU-local constants rather than unexplained literals.

## Honest fuzzy park

`game_state_open_persistent_storage` is retained as the best credible complete
body with **zero exact credit**:

- target: 395 meaningful / 400 padded bytes, 26 relocations, normalized SHA-256
  `b8779377c24f612ca3559f6a8f5fd75277d2c541f847f8c331d79cceabd6769b`;
- candidate: 400 padded bytes, 26 relocations, normalized SHA-256
  `6457f7e47a8b11aa911b5099e4bce81a274621530d30235c8909cce4d6b09cbc`;
- ordinary objdiff similarity: 91.22951%;
- relocation destination sequence: equal;
- remaining difference: terminal cold-block placement and absorbed padding.

A later single-exit probe found a superficially closer arrangement only by
adding a handle initializer instruction absent from January. It was rejected
under the no-fake-matching rule. Seven other natural control-flow forms either
returned to this fixed point or made the candidate worse. The retained body is
therefore parked as `instruction-scheduling` until authoritative source or a
compiler-local record explains the terminal block tie.

## Focused verification

The packet was compiled with the PID-safe one-TU gate, three temporary aliases
for the pre-rename target symbols, and an emitted-symbol guard:

```text
python -B tools/campaign/gate.py "source/saved games/game_state_xbox" \
  --source scratch/game-state-xbox-remainder-20260908/candidate-game_state_xbox.c \
  --alias game_state_get_persistent_storage_path=code_001b0220 \
  --alias delete_persistent_storage=code_001b0240 \
  --alias game_state_open_persistent_storage=code_001b0270 \
  --all --out scratch/game-state-xbox-remainder-20260908/candidate-final.obj \
  --forbid-emitted-symbol point_from_line3d
```

Result: emitted-symbol guard passed; 16 exact / 1 residual / 0 unwritten.
`tools/coff_compare.py` reports `all_equal: true` across all 16 exact owners.
`tools/fake_match_scan.py --fail-on-findings` reports zero findings. A
pre-rename minimal parked-ledger harness reports one active, zero stale and
zero invalid entries.

This is a focused packet only. The orchestrator must regenerate configuration,
run the full build, stable all-owner sweep, parked audit, admission audit and
test suite before committing or publishing it.
