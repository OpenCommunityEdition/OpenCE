# Measured safe harvest, 2026-08-20

This Codex-owned ledger records a bounded seven-object harvest from the
authoritative campaign commit `8b7a83037da304625ff54a90ae9e2f3b2dacd3fc`.
Every retained source change is a previously measured, target-backed function
or a same-size typed field needed by one such function. All seven objects stay
`NonMatching`; no configuration, semantic exception, or parked-function entry
changed.

No existing or Claude-authored Markdown was edited or deleted. The only
Markdown change in this lane is the addition of this ledger.

## Retained donor scope and commit stack

| Object | Historical donor | Integration commit | Retained scope |
| --- | --- | --- | --- |
| `props.obj` | `e679221bc541fe8d2a0755c565a6debd72496960` (originating `codex/props-full-admission` lane at `1d0d1c...`) | `8a988dfaaef976062a9d9142b67803aba95cb22f` | Only the branch polarity in static `code_00052a70` |
| `thread_win32.obj` | `fa810389eef3b1d0fa68eee18c55235682a5b073` | `50607fa2d7e09d8dd1bbeb607690f82bd8e5b2c6` | Only `create_mutex` |
| `path_structure_bsp.obj` | `af85c55ad3404d774bb381b784f8516a6571fb35` | `f16ce0b5b3b480780391b8a39d6af6f505e8cd69` | Only `clip_empty_interval_by_solid_interval` and its typed includes |
| `player_control.obj` | `5bc9fb4ea759e1ed641c40d6b38cff2174143beb` | `d0e42a54e9932bd20849f959127a3de1cb36a16b` | Only `unit_get_local_player_index` |
| `stack_walk_windows.obj` | `e022615f10a851e7ca97ede31fa10babd1eefc1a` | `61d83d47ce2d78afc9eff8f0f346c2b3aed9e210` | Only `stack_walk_global_function_offset` |
| `input_xbox.obj` | `e022615f10a851e7ca97ede31fa10babd1eefc1a` | `3d843b8d15512d771a3a997be5f16b4d0ebdc4db` | Only `code_000bebb0`, `code_000bebc0`, `input_suppress`, and the same-size typed `suppressed` byte |
| `ui_widget.obj` | `86235def10081f3352955e8c79cf5d94c00a8b5f` | `05a16d19dbe3e4465403af9217ba99ecb618dbc7` | Only target-backed no-op functions `code_000d4680` and `code_000d4690` |

## Strict results

The table reports the hardened semantic-COFF accepted set after a fresh build.
Meaningful bytes exclude section padding; padded bytes and relocation counts
cover every exact function in the object. Data is the strict target-owned
runtime `.data`, `.rdata`, and `.bss` boundary, excluding directives and debug
sections.

| Object | Exact functions | Meaningful code | Padded code | Relocations | Exact runtime data sections | Exact runtime data bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `props.obj` | 10/17 | 757 | 832 | 47 | 13/13 | 937/937 |
| `thread_win32.obj` | 8/9 | 655 | 720 | 48 | 7/8 | 149/1,693 |
| `path_structure_bsp.obj` | 1/6 | 104 | 112 | 0 | 0/6 | 0/194 |
| `player_control.obj` | 37/49 | 2,645 | 2,896 | 154 | 10/33 | 272/2,164 |
| `stack_walk_windows.obj` | 4/16 | 177 | 208 | 14 | 3/34 | 77/17,174 |
| `input_xbox.obj` | 8/27 | 26 | 128 | 2 | 0/20 | 0/2,716 |
| `ui_widget.obj` | 3/102 | 3 | 48 | 0 | 0/206 | 0/26,433 |

The measured gain is ten strict-exact functions, 636 meaningful code bytes,
752 padded code bytes, and 37 relocations. Runtime data credit is unchanged.
The new functions themselves measure as follows:

| Object | Function gain | Meaningful gain | Padded gain | Relocation gain |
| --- | ---: | ---: | ---: | ---: |
| `props.obj` | +1 | +315 | +320 | +22 |
| `thread_win32.obj` | +1 | +132 | +144 | +10 |
| `path_structure_bsp.obj` | +1 | +104 | +112 | 0 |
| `player_control.obj` | +1 | +56 | +64 | +3 |
| `stack_walk_windows.obj` | +1 | +17 | +32 | +1 |
| `input_xbox.obj` | +3 | +10 | +48 | +1 |
| `ui_widget.obj` | +2 | +2 | +32 | 0 |

## Verification

- Each object was force-rebuilt and each named function passed the hardened
  byte, padded-size, relocation-address/type/destination, and normalized-hash
  comparator before its commit.
- A clean post-integration regression snapshot covering all seven translation
  units was followed by a forced clean/rebuild of all seven objects. The
  fail-closed comparison passed with zero failures, zero warnings, no changed
  nonexact functions, and every accepted function still exact.
- The full 567-step Halo and libcmt build/report graph passed. The repository
  semantic report scans 470 units and 3,873 functions, finds 3,732 semantic
  exact and 3,796 accepted exact functions, and reports zero unit errors.
- Campaign progress is 375/833 objects, 3,785/11,060 functions,
  460,241/2,198,102 code bytes, and 1,803,160/4,176,062 data bytes.
- Admission reports zero candidates and zero revocations; the sole inherited
  contradiction remains `source/shell/shell_xbox`. Park validation reports
  three active, zero stale, and zero invalid entries.
- The complete tooling suite passes 179/179 tests.
- A cross-worktree frozen-object regression against the authoritative base
  found no changed exact or nonexact function and no runtime-code, relocation,
  runtime-data, or nondebug symbol-ownership change. The only raw-object
  differences were expected `.debug$S` compile-path records from the distinct
  worktree directories.

## House and scope audit

The retained source is readable typed C. Added source contains no inline
assembly, volatile scheduling device, forced inline, optimizer pragma or
barrier, undefined type punning, byte forcing, raw layout cast, or per-object
compiler flag. New no-argument functions use an explicit `void` parameter;
parameters are one per line; every retained function ends in an explicit
return. The input layout change replaces bytes inside one translation-unit
local placeholder without changing the `input_globals` size or the offsets of
existing fields, and the only repository reference to that structure remains
inside `input_xbox.c`.

The frozen objects `vehicles.obj`, `ai_debug.obj`, `ai_script.obj`,
`actions.obj`, and `units.obj` were not edited. The lane also explicitly
excludes the later `_ReadWriteBarrier`/`create_thread` form, `hud_draw`,
`get_return_eip`, assembly donors, `real_math`, `lruv`, `/QIfist`, and all
nonexact or unsafe neighbours. The `errors`, `real_math`, `unpark.py`, and
parked-ledger portions of the `path_structure_bsp` donor were not taken. The
optional `rasterizer_text`, `models`, and `items` sibling stubs were not taken.

The final tracked scope is seven C files plus this new Codex ledger, with no
deletions. No GitHub push was performed.
