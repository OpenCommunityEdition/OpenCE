# main.obj static-storage packet, 2026-10-01

Lane `claude/remaining-frontier-20260926`. Independently reconciled on canonical on 2026-10-02;
see `upstream_tmp_reconcile_20261002.md` for the fresh clean-build gate and canonical measurements.
Owner rulings Q-UP4 (measure), Q-UP5 (land as measured) and Q-UP6 (names), 2026-10-01. Donor shape:
punpckhdq/halo pull request #84 (`a5ad6b2c`, CC0), rewritten with the lane's text and checked against January.
Research record: `research/remaining_frontier_20260926/lead/upstream_punpck/cards/UP-10_main_static_storage.md`.

| Function | Meaningful bytes | Padded bytes | Relocations |
| --- | ---: | ---: | ---: |
| `_main_frame_rate_debug` | 538 | 544 | 52 |

## What changed

- `source/main/main.c`
  - The packed 0x38B-byte `struct _screenshot_and_framerate_globals` (with `byte reserved002[6]` and a `pack(1)`
    pragma) and its size assert are removed. January has nine separately aligned objects there.
  - `short global_screenshot_count = 0;` at file scope. Three other units already declare it `extern short`, and
    January's script-global table types it `_hs_type_short_integer`.
  - `main_game_render`: `static struct render_window window[MAXIMUM_LOCAL_PLAYERS + 1] = { 0 };`. The function
    moves up, directly after `main_get_window_count`; `screenshot_render` gets a prototype because it is now
    called before its definition. The pointer local is renamed `current_window`.
  - `main_frame_rate_debug`: a local enum (`NUMBER_OF_FRAME_SAMPLES = 8`, `RUNS_BEFORE_RESET = 60`), six static
    locals, and the natural body: `if (need_to_initialize && !debug_frame_rate) { clear }` then
    `if (debug_frame_rate) { ... }`, with direct access to the statics. The hand-threaded `sample_index` local, the
    two early returns and the `(char)(... % (long)NUMBEROF(...))` casts are gone.
  - `halt_and_catch_fire`: `static boolean recursion_lock = FALSE;`.
- `config/symbols.json`: eight `"static": true` rows for the January addresses of the new statics (line surgery,
  directly after `_global_screenshot_count`).
- `config/parked.json`: the `_main_frame_rate_debug` park is retired.

No padding member, filler declaration or pragma is added. No compiler flag, scorer or credit rule changes.

## Evidence

| Claim | Status | Basis |
| --- | --- | --- |
| The public symbol is a `short` | proven | word-width access at +0 in ten functions of three objects; the script-global table |
| The span is nine objects, not one aggregate | proven by a controlled ablation | the same body on the packed struct is residual at all 64 C1 shifts: VC7 keeps a scalar static in a register across the two `if` statements (January's `xor dl, dl`, `mov dl, [index]`, `movsx ecx, dl`) and reloads a struct member from memory |
| Sizes, widths, signedness and `.bss` offsets (+0x6D8, +0x6E0, +0xA3C, +0xA5C, +0xA5E..+0xA62; section 0xA63) | proven | January's references and access widths; the rebuilt object has every object at January's offset |
| The 6-byte hole at +0x6DA | explained | ordinary 8-byte alignment of the array; nothing references it anywhere on the board |
| Internal linkage of the eight statics | proven | January's PDB lists no public symbol in the span |
| Explicit zero initialisers and the declaration order | measured by perturbation | without the initialisers the statics move to the front of `.bss` in name-hash order (section 0xA65) |
| Two stores of zero to the reset flag | proven | January +0x25 (before the `csmemset` call) and +0x4E |
| The values 8 and 60 | proven | 0x20-byte array, `and eax, 0x80000007` with the signed fixup, `cmp al, 0x3c` |
| `window`: name, type, function scope, local players + 1 | attested by later first-party builds | PC demo PDB 2003 (`[2]`), HCEX PDBs 2011 (`[3]`); January has five |
| `recursion_lock`: name, type, function scope | attested by a later first-party build | HCEX_Release.pdb 2011 |
| Function scope of the six frame-rate statics | INFERRED | each is referenced by this one function; the two attested neighbours are function-local; file scope gives identical bytes |
| The six frame-rate names and the two constant names | INFERRED descriptive names, not recovered Bungie names | no later build keeps the function body; every spelling gives identical bytes |
| `main_game_render` before `main_frame_rate_debug` and before `screenshot_render` | measured | `.bss` declaration order; January emits the function one sweep after `screenshot_render` (VC7 emits callee first, in repeated textual sweeps) |
| The exact line of `main_game_render` | INFERRED | PR #84's relative position, directly after `main_get_window_count` |
| `current_window` | descriptive | the attested static takes the name `window`; no PDB records the pointer local |
| The threshold spelling `1.08/TICKS_PER_SECOND` | unchanged, a disclosed choice | it folds to January's constant |

## Not part of this packet

- `scenario_paths` (a function-local static in the 2011 PDB only): untouched.
- Precompiled-header settings: untouched. January's `main.obj` function order shows the precompiled-header
  pattern; that is recorded as research evidence only.
- `main`'s object admission: untouched. `main` stays NonMatching; its `.data` keeps the 2026-09-27 semantic
  exception (+52).
- The lane's function order elsewhere in `main.c`, and the later-PDB local names of `main_game_render`.

This is a narrow packet approval, not a general permission for function moves or guessed globals.

## Original lane gate

Measured first in a shadow of lane HEAD `78294dd3` (checkpoint `SH_VA` against `SH_P`), then gated fresh in the
lane: `R114i_M` against `R113i_P` (incremental build, objdiff 3.3.1). Zero exact losses.

- Stable diff: gained 1 (544 padded bytes), regressions 0.
- Functions 7,479 -> 7,480 of 7,573. Code 1,601,295 -> 1,601,833 of 1,770,166 (+538 meaningful bytes).
- Data 3,311,591 / 3,922,163 and objects 396 / 468 unchanged.
- Strict section owners 7,652 -> 7,653 of 8,252.
- Parks 62 -> 61, 0 stale, 0 invalid. Admission candidates 8 -> 9 (`source/main/main` becomes an audit candidate
  only; no status flip), 0 contradicted, 0 rejected.
- Tools tests 1,700 passed, 5 skipped. Fake-match scan: the same 26 inherited leads. Whitespace checks pass.
- One object differs on the whole board, `main.obj` (1 of 621). By content, three code sections differ:
  `_main_frame_rate_debug` (the gain) and `_halt_and_catch_fire`, `_main_game_render`, whose relocation targets
  change from `_global_screenshot_count+N` to the statics' own symbols with the same destinations. Both stay
  exact. Six sections change position with identical content: `_main_game_render`, `_create_local_players` and
  the latter's four assert literals. The compiler emits the moved function later, so the two swap places.
- /W3 4,082 = 4,082, no unit differs. /W4 for `main.c` 90 = 90.
- Without the eight symbol rows `main` `.bss` would score 82.94% and the progress step would fail; they are part
  of the packet.
- C1 shift sweep (diagnostic only): `_main_frame_rate_debug` is exact at all 64 shifts, so the result does not
  depend on a declaration count.
