# `player_rumble.obj` typed update recovery (2026-08-27)

This ledger records one strict-function recovery in
`source/game/player_rumble.obj`. The object advances from 10/12 to 11/12
strict functions and from 768/1,488 to 1,072/1,488 strict padded bytes.
`_code_000a91a0` remains nonexact, so the object remains `NonMatching` and no
whole-object completion is claimed.

Repository strict totals advance from 4,337 to 4,338 functions and from
562,071 to 562,375 padded bytes. The whole-object total remains 268/619.

## Authority and provenance

- Local committed base: `540093129a5a0b8084e7e8ffb0f07747a43d0140`.
- Baseline source blob: `952eab15fc841cfbe3d7f47b31ae1f04f80fe7eb`.
- Retained source blob: `93b28f3385935647f0a1a4739b8c1e225ed908c1`.
- Retained source SHA-256:
  `80a1d250023d3fb6a0924e27656525cab9b0283e23eed7f4c116215d8567e0ba`.
- January object SHA-256:
  `c6f3cadcd7081dd4ffff4fa6ff9abbfb9c1badebac05c22811712c6daee97db5`.
- Preserved candidate object SHA-256:
  `cfef7f3611581b38f1462eaf789268f86c89849addc4e7ebd54cc06e70193024`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged production edge.

The starting ten-function checkpoint and the original full reconstruction were
reviewed in full:

- `docs/object_matching_logs/player_rumble_obj_codex_checkpoint.md`;
- historical `5dee3760:docs/object_matching_logs/player_rumble_obj.md`;
- source history commits `c2a6d850` and `5dee3760`.

A strict cross-branch sweep compiled every distinct historical
`player_rumble.c` blob in isolated scratch space. Blob
`c5b96f7d2d6a82cd37b345a6609c97d7c8b31702`, retained on
`agent/cluster-partitions-admission` and
`agent/hashtable-admission-audit-20260811`, was the sole improving donor. It
moved the unit from ten exact / two unwritten functions to eleven exact / one
residual function.

## Typed recovery

The historical donor makes `_rumble_update` exact only when the private mixer
is defined in the same translation unit, allowing VC7 to use January's private
EAX argument convention. The donor's raw
`datum_get(player_data, player_index) + 2` access was not retained.

The production source includes the real player, input, player-UI, periodic
function, and game-state headers and expresses the same signed 16-bit field
load through the typed subsystem macro:

```c
player_get(player_index)->local_player_index
```

The typed form is code-neutral and satisfies the campaign rule against raw
datum access plus pointer arithmetic. All no-argument functions use `void` on
its own line, parameters are one per line, and every void function ends with
an explicit `return;`.

## Strict recovery

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rumble_update` | 304 | 16 | `0149be7a3b1ffa37395a640eec09e8d2fb6f2587ec69ef3aa3e5572b712efd07` |

Every padded byte and every relocation address, type, destination, owner, and
addend match January. The sixteen relocations include the private mixer call,
eight timer increments, typed `player_get` expansion through `_player_data`
and `_datum_get`, the player-UI queries, and the rumbler-state output.

The ten previously accepted functions remain strict-exact. The four-byte
`_bss_00453404` owner also remains exact with normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.

## Honest residual boundary

| Function | Target / candidate bytes | Target / candidate relocations | Target / candidate normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_000a91a0` | 416 / 416 | 12 / 19 | `ee97f132ea073644aada4775c0d5f96149754b0ad66a2926fb458e4ae4b30b7e` / `0616035255a76eb5cc73a1e33bf7778b863af59fc1d9c79c8d42d56fa5328d73` |

The readable mixer preserves the recovered behavior and exact 416-byte
envelope but the ordinary compiler edge lowers the final float-to-word
conversions through three `__ftol2` calls and additional constant relocations.
January uses direct x87 integer stores. Historical `/QIfist` analysis did not
close the remaining constant-ownership gap and compiler-flag substitution is
prohibited. The source documents this boundary and does not use assembly,
`volatile`, `register`, pragmas, intrinsics, attributes, barriers, raw
addresses, representation tricks, undefined behavior, object patches, or
comparator/admission exceptions.

## Frozen compile and gates

The final production candidate ran once and was immediately preserved:

```text
[1/1] CL build\base\source\game\player_rumble.obj
player_rumble.c
```

Preserved artifact:
`build/audit/player_rumble_typed_update_house_first_shot_20260827.obj`.

Full validation passed:

- Halo and libcmt builds, split, ordinary report, semantic report, and strict
  progress;
- strict board: 268/619 whole objects, 4,338/8,246 functions, and
  562,375/1,922,669 padded bytes;
- `player_rumble.obj`: 11/12 functions and 1,072/1,488 padded bytes;
- semantic report: 470 units, 4,469 functions evaluated, 4,347 semantic exact,
  115 hidden / 64,889 hidden bytes, 4,375 accepted, and zero unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked audit: three active, zero stale, zero invalid;
- tooling suite: 194 passed, with only the known unwritable pytest-cache
  warning;
- `build/report.json` SHA-256:
  `f546d5f59316cf096a363d6cdc037236fa8055c767c34c463ec15dcb55240073`;
- `build/semantic_report.json` SHA-256:
  `a43bff6a8db264cbebfba09073c40690edcf88a06463f635bcab8561fa631ae3`;
- `git diff --check`: clean apart from Git's line-ending notices.

No tool file changed; the tracked `tools/**` inventory remains 121 paths.
