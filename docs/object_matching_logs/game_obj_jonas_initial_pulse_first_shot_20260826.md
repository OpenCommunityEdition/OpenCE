# `game.obj` Jonas initial-pulse first shot (2026-08-26)

This bounded Jonas/Codex wave starts from clean cumulative commit
`6296c9960d4f07e3f695fad765c2a69566523b75`. The first and only production
candidate compile makes `_game_initial_pulse` strict exact while preserving all
six previously accepted `game.obj` functions. The unit advances from 6/27 to
7/27 exact functions and from 114/3,217 to 185/3,217 meaningful code bytes.
`game.obj` remains `NonMatching`; no whole-object or runtime-data claim is made.

## Scope and guard boundary

The implementation scope is only `source/game/game.c`. It adds the canonical
16-byte data iterator declaration, typed local declarations for the two
game-engine calls and `player_data`, and one ordinary iterator loop. The public
`game.h` declaration and the existing caller in `main.c` already agree with the
external cdecl `void(void)` ABI, so neither requires an edit.

The Units shared-header package remains unapproved. No protected file is
touched: `source/units/units.c`, `source/units/units.h`,
`source/game/game_engine.c`, `source/game/game_engine.h`,
`source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, and `source/ai/actions.c` all remain byte-identical to
the base commit.

## Why this was the only eligible owner

The earlier immutable
`docs/object_matching_logs/game_obj_jonas_typed_continuation_20260821.md`
proves that this apparent six-leaf cluster was not fresh. One production wave
already rejected `_game_map_loading_in_progress`,
`_game_difficulty_level_get_ignore_easy`, `_game_all_quiet`, and
`_game_safe_to_speak`. That same record excluded `_set_random_seed` before
compilation because `game.h` includes the shared `__inline` definition in
`math/real_math.h`; recovering the neighboring out-of-line owner would require
a shared-inline semantic change or synthetic emission. None was retried here.

An all-ref history search, object-ledger census, registered-worktree search,
and two independent read-only audits found no earlier definition or production
attempt for `_game_initial_pulse`. The baseline object does not define it. This
made the pulse the packet's sole genuinely fresh owner.

## Immutable inputs and artifacts

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `game.obj`: 27,254 bytes, SHA-256
  `fd703ba666f13a5f1311dfbe1383eba452cf49a0a9a7fab47b1cfbfd06c1cfd0`.
- Baseline source blob: `b900496902dfe978beafe03e80af7005b7d7a4ec`.
  Its checked-out payload is 6,011 bytes, SHA-256
  `3c0112a3bec47104543b18183627f86b68e687428906b7dcede9b0336def90a8`,
  with 225 CRLF and zero bare-LF endings.
- Retained filtered source blob before commit:
  `22040601a6eec509ed822e9420089e36f4e29e75`. Its checked-out payload is
  6,433 bytes, SHA-256
  `fad9b96ac9b6f8318d0c97cc873aee16e8fa43422ad3e7c5abadd07fafb1d27f`,
  with 246 CRLF and zero bare-LF endings.
- Stable untouched baseline object: 1,808 bytes, SHA-256
  `7005117bedcbfd34d47eaa90db210c21d4de007e13788e0b450d2976398052f8`.
- Preserved first candidate object: 2,257 bytes, SHA-256
  `b3f16381dd96a56ef10292e107bb37d7668764a425a35d80a44bb173f84fb091`.
- Carried-forward clean `game.obj` regression manifest: 551,382 bytes,
  SHA-256
  `d9d5b9eeb217f68b5705ea9d583aa6ac0ba82143b744fd47e4f711927d217e15`.
  It was captured at accepted game-source commit `6a68f014`; every later commit
  through this wave's base leaves `source/game/game.c` unchanged.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 from
  XDK 3911, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Authenticated `build.ninja` and `objdiff.json` SHA-256 values are
  `6123636fac2ddfe09196d95f7e8618a32db0603eac1d59f3771c0e78fc1bcff2`
  and
  `32014f8b48817a1548aa66c85003ac939f42d8518047fba2af770395d97866a6`.

## Claude and supplied-repository evidence

January COFF remains the only byte, relocation, section, padding, and linkage
authority. The duplicated 47,280-byte campaign `CLAUDE.md` payloads have
SHA-256
`c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`;
their aligned 44,094-byte `AGENTS.md` payloads have SHA-256
`b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`.
The reviewed later-source `CLAUDE.md` is 37,778 bytes with SHA-256
`97d4c63b9dd9d642cbe6774c5f3a578c60f2de002fce3ac888cd4ee635cc4c3d`.
The applicable doctrines are binary-first ABI/layout evidence, C89-compatible
typed source, explicit unknowns, small changes, and no inline assembly. Its
later patching/lift pipeline targets a different binary and grants no matching
credit here.

The user's supplied repositories were authenticated at immutable revisions:

- [`punpckhdq/halo` at `fc47f9a`](https://github.com/punpckhdq/halo/blob/fc47f9a15a0aea7f0081394c4d66db5c68ab5323/source/game/game.c)
  supplies the same-build symbol order and the pulse's 0x50 padded boundary;
  its `game.c` is a symbol skeleton, not a body donor.
- [`stianeklund/halo` at `41c106`](https://github.com/stianeklund/halo/blob/41c10616b69b982700e0913f21a5137807a03d0e/src/halo/game/game.c#L410-L418)
  supplies the readable player-iterator loop. Its independent VC71 score ledger
  records this body at 100%, but the older Xbox build remains source-shape
  evidence only.
- [`halocea` at `570c83f`](https://github.com/surreptitiousresearch/halocea/blob/570c83fd9c365dad6f2a3e7041705d5b84c7847c/src/blam/game/game_initial_pulse.c)
  independently corroborates call order and a `void` result. Its iterator
  header places the full 32-bit datum handle at `+8`; its own README labels the
  PowerPC reconstruction non-byte-perfect, so it is type/semantic provenance
  only.
- [`pastudan/halo` at `918af88`](https://github.com/pastudan/halo/blob/918af885935ec470a31256ecce9a977b12b01f80/src/halo/game/game.c)
  repeats the Stian source and is not counted as independent evidence.
- `halopc-restored` contains tag/script restoration rather than engine bodies,
  so it contributes no production source to this wave.

The XboxRecomp PR and the linked Baboon, Demon, Nimbus, HCEA-docs, and reverse
engineering repositories were also audited for tooling ideas. The directly
applicable follow-ups are a strict-past-target CFG boundary sidecar, reference
slice integrity checks, a nonce-hardened oracle/candidate golden runner, and
selective unresolved-call/ESP tracing. They are deliberately deferred to
separate tooling changes: the existing hardened COFF comparator remains this
wave's admission authority, and no external code or license surface is copied.
The pinned `run_golden_xbox.py` identifies itself as legacy; no new caller is
added.

## Binary and ABI closure

January defines `_game_initial_pulse` as an external storage-class-2/type-0x20
cdecl `void(void)` owner with 71 meaningful and 80 padded bytes. Its exact
relocation schedule is:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+7` | `DIR32` | `_player_data` |
| `+17` | `REL32` | `_data_iterator_new` |
| `+26` | `REL32` | `_data_iterator_next` |
| `+42` | `REL32` | `_game_engine_player_added` |
| `+51` | `REL32` | `_data_iterator_next` |
| `+63` | `REL32` | `_game_engine_game_starting` |

The target's `mov -8(%ebp), %eax` reads the iterator's full `long datum_index`
at offset `+8`. Current `memory/data.h` proves the iterator is exactly 0x10
bytes and supplies typed declarations for both iterator calls. The local
game-engine forwards agree with their protected definitions without including
or editing the protected header. `_data_iterator_new`, `_data_iterator_next`,
`_game_engine_player_added`, and `_game_engine_game_starting` all independently
compare strict exact against January.

## Frozen one-shot discipline

The isolated worktree was created at the exact cumulative base. During
untouched setup, the first baseline build attempt exposed the omitted copied
XDK support directory and failed before emission; after the authenticated XDK
was copied, the stable baseline compiled and all six sentinels passed. A
pre-wave snapshot retry then exposed stale copied graph timestamps and stopped
on a network-denied tool refresh while compiling only unrelated baseline
objects. This was before source mutation and emitted no candidate code.
`ninja -t restat` restored the copied graph; `all_source` then reported no work.

After the source packet was frozen, `git diff --check` and the exact production
flag `/Zs` invocation passed. The dry run showed exactly:

```text
[1/1] CL build\base\source\game\game.obj
```

That ordinary edge was invoked once. There was no declaration, spelling,
control-flow, source-shape, flag, or compiler-tuning retry. The patch tool had
introduced 37 bare-LF separators among otherwise CRLF text; after the first
artifact was preserved, a purely mechanical newline normalization restored all
246 separators to CRLF. No token or AST changed, and no second uncommitted
candidate compile was performed. The clean committed-state replay will compile
the retained all-CRLF payload.

## Strict comparison result

The hardened comparator reports `all_equal: true` for the new owner and all six
sentinels:

| Function | Padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_game_options_new` | 48 | 1 | `38dc2b2eccc7f378ebffdabbd7b8d3c0efeba231c33bc1385d99538d96443ae5` |
| **`_game_initial_pulse`** | **80** | **6** | **`7e8c9a85b8111434eba409977b9dd10e583296c47632cabb603375fa04690faa`** |
| `_game_set_players_are_double_speed` | 32 | 1 | `49422947cae7e2aeda7d16a5ae102b9a041fd9b31c52a6eded48ca6f3912e850` |
| `_game_players_are_double_speed` | 16 | 1 | `17af9f50995ca94c2ba3d79997b48bd5fc6f56bc6053b77e817c7dcc6654b8a6` |
| `_game_difficulty_level_set` | 32 | 1 | `fa722c4cadca20745287ef0441121e91fbd376bccc674af844dd67bd4c16da45` |
| `_game_difficulty_level_get` | 16 | 1 | `d6bf822a580de3673043267fa11c2fa4d0ea26bdd7991192f3225bb77930ddc3` |
| `_game_is_cooperative` | 16 | 1 | `dc1664259fae636950aeaf08e39776766f544d0b4ac207d6912031ef2a9dceea` |

The candidate owns exactly these seven external functions. The only newly
defined external symbol is `_game_initial_pulse`; the five new undefined
dependencies are `_player_data`, `_data_iterator_new`, `_data_iterator_next`,
`_game_engine_player_added`, and `_game_engine_game_starting`. The object emits
no `.bss`, `.data`, or `.rdata`, so no January runtime-data span receives
credit.

The generic carried-forward regression check correctly reports only
`_game_initial_pulse` under `newly_exact` and `changed_nonexact: []`. It fails
closed on five inherited accepted fingerprints, compiler-debug sections, and
symbol inventory because the inserted function COMDAT renumbers later local
sections. No adjudication or waiver is added. Direct symbolic-relocation-aware
comparison above proves all seven accepted owners unchanged/exact.

## Full validation and campaign progress

The complete Halo and libcmt object graphs pass and then report no work.
Canonical outputs are:

- `build/report.json`: 1,574,841 bytes, SHA-256
  `9856b039c85d9ff81c5da851e95d390896550a9a8c991befd4b92b78c7512498`;
- `build/semantic_report.json`: 2,986,342 bytes, SHA-256
  `005d4c02b6cc2979a288beda3b3bd6c1d160c7f0098abe89dbf11e6ba449219d`.

Campaign progress becomes:

- all categories: 375/833 complete objects, 4,159/11,060 exact functions,
  502,692/2,198,102 meaningful code bytes, and
  1,835,212/4,176,062 data bytes;
- Halo: 273/468 complete objects, 3,992/7,574 exact functions,
  489,778/1,770,166 meaningful code bytes, and
  1,830,020/3,923,451 data bytes;
- libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The strict semantic audit scans 470 units and evaluates 4,266 functions:
4,124 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36
ordinary-only, 35 structural, one rejected, 12 local skips, 3,463 missing,
4,185 accepted exact, and zero unit errors.

Admission remains zero candidates and zero revocations, with only the inherited
unrelated `source/shell/shell_xbox` completion-label contradiction. Its 384-byte
JSON has SHA-256
`f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`.
Parks remain three active, zero stale, and zero invalid; the 3,950-byte JSON has
SHA-256
`cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.
All 179 tooling tests pass, and all 14 tracked/configuration/canonical-report
JSON documents parse.

## Scope and policy

The retained source is readable typed C89-style code. It uses the named
`data_iterator` fields and contains no assembly, raw address or byte-offset
access, pointer/integer reconstruction, pointer or union pun, inactive-union
access, undefined signed overflow, volatile or `register` scheduling device,
force-inline annotation, optimizer pragma or barrier, synthetic anchor,
object-byte forcing, or compiler-control change.

Before the ledger is added, tracked scope is exactly `source/game/game.c` with
no deletion and no protected-path intersection. No header, configuration,
semantic exception, parked record, completion label, pre-existing Markdown,
Claude-owned file, or build rule changes. No push, amend, rebase, history
rewrite, or worktree removal is performed.

## Committed-state replay

The clean committed-state manifest, forced selected-object replay, exact
comparison, full reports, tests, and cumulative integration replay will be
recorded append-only after the implementation-and-ledger commit.
