# `game_sound.obj` Opus scripting-pair reconciliation (2026-09-04)

## Scope and evidence

- Canonical base: `0a211a992`.
- Frozen donor branch: `opus/small-families-30k-20260902`, commit
  `8aed00b49b8b77188dc3aa2ce213ade5440f4e74`.
- January target owner: `build/split/source/sound/game_sound.obj`.
- Stable-verdict baseline:
  `scratch/after-game-safety-pair-20260904.json`.

The earlier `game_sound` one-shot ledger rejected the then-emitted
`scripted_sound_time` and parked the public looping-stop wrapper because their
natural forms did not match in that older translation-unit context.  The
frozen Opus commit is materially new evidence: after later, independently
accepted type/interface work, the same ordinary typed topology emits all three
owners strict-exact with the canonical VC7 toolchain.  This reconciliation
copied only that bounded packet and re-ran it on current canonical; no bulk
cherry-pick or donor mutation was used.

The pinned HCEA tree at `c168af2e747d3095d9a29418ae401f3a39544863`
independently supplies `src/scripted_sound_time.c`,
`src/scripted_looping_sound_stop_internal.c`, and
`src/scripted_looping_sound_stop.c`.  It corroborates the names and semantic
topology only; January's COFF remains authoritative for PC layout, ABI,
instruction bytes, and relocations.

## Retained exact owners

| January owner | Padded bytes | Result |
| --- | ---: | --- |
| `_scripted_sound_time` | 80 | strict exact |
| `_code_001b7790` -> `_scripted_looping_sound_stop_internal` | 112 | strict exact after semantic/static symbol recovery |
| `_scripted_looping_sound_stop` | 96 | strict exact |

Total retained gain: **3 functions / 288 padded bytes / 262 meaningful code
bytes**, with zero stable-verdict regressions.

`scripted_sound_time` resolves the typed sound definition, returns zero for a
missing definition or inactive scripting timer, and otherwise clamps the
remaining tick count at zero.  The private stop helper resolves the typed
looping definition and datum, clears the scripted flag through `SET_FLAG`,
requests the normal unattached stop, clears the definition's live scripting
index, and optionally selects fixed fadeout.  The public wrapper passes
`FALSE`.  No raw offsets, casts after raw tag/object access, fabricated
dependencies, or matching-only control flow are retained.

The anonymous target name was replaced in `config/symbols.json` with
`_scripted_looping_sound_stop_internal` and `"static": true`.  The mapping is
supported by HCEA's independently named function and the frozen donor's unique
112-byte/five-relocation name-gap pairing.

## Declaration ownership

`scripted_sound_time`, `scripted_sound_new`, and `scripted_foley_predict` now
live in the owning `source/sound/game_sound.h`, alongside the already-owned
looping-sound API.  `source/hs/hs.c` and
`source/ai/ai_communication.c` include that header and no longer carry local
copies of those game-sound declarations.  The touched declarations use one
parameter per line.

Because `game_sound.h` contains type definitions and C2 is sensitive to
definition position, every transitive consumer was rebuilt.  No strict-exact
function regressed.  The include changes only the existing
`ai_communication_update_speech_timers` fuzzy residual's register schedule:
its size remains 672 bytes and all 43 relocation identities remain intact;
its honest score moves from 97.76382% to 97.67839%.  The corresponding
`config/parked.json` measurement and evidence were refreshed rather than
silently accepting stale park data.

## Verification

- Focused gates:
  - `source/sound/game_sound`: **13 exact / 0 residual / 18 unwritten**
    (from 10 / 0 / 21).
  - `source/hs/hs`: **445 exact / 3 residual / 0 unwritten**, unchanged.
  - `source/ai/ai_communication`: **37 exact / 1 residual / 10 unwritten**,
    unchanged.
- Stable verdict diff:
  `scratch/after-game-safety-pair-20260904.json` ->
  `scratch/after-game-sound-opus-owned-20260904.json`:
  **+3 / +288 padded bytes / 0 regressions**.
- Full Ninja build: pass; semantic report **5,954 accepted exact**, zero unit
  errors.
- Project progress: **843,920 / 2,198,102 meaningful code bytes** and
  **5,885 / 11,060 functions**.
- Parks: **188 active / 0 stale / 0 invalid**.
- Object admission: zero candidates, contradictions, or revocations; two
  inherited explicit rejections.
- `tools/fake_match_scan.py`: zero review leads across all four touched
  C/header files.
- Tool tests: **261 passed**.
- Protected bitmap working files retained their pre-existing SHA-256 values.

No inline assembly, `volatile`, register forcing, optimizer barrier, pragma,
undefined behavior, representation pun, raw address, fake dependency, forced
inline/noinline, or nonsensical branch was added.  The January inline schedule
remains intact and the rebuilt touched objects emit no
`point_from_line3d` COMDAT.
