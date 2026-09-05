# Player UI owner and house-rule reconciliation — 2026-09-04

## Disposition

Baseline is published `d556ba5304eeb4ee663b7c81998925b36f2d5cd7`.
The independently compiled baseline and each bounded source reconciliation
retain **42 strict-exact target functions**, 3,989 meaningful / 4,320 padded
code bytes, with no residual, unwritten function, additional code owner, or
emitted `point_from_line3d`. This packet adds **zero new exact code bytes**.
It improves source fidelity and data-owner naming; it does not authorize a
`Matching` label. The remaining public playlist layout and the failed full
object link experiment are explicit boundaries below.

Only `source/interface/player_ui.c` and this ledger were edited by this lane.
No header, configuration, Ninja output, commit, or push was changed. Root owns
the symbol-map integration and final aggregate validation. The prior cumulative
ledger is `docs/object_matching_logs/player_ui_obj_codex_checkpoint.md`.

## Natural source reconciliations

1. Replace the external address placeholder with
   `static long player1_last_used_profile_index = NONE;` and update its four
   source uses. The identifier is supported by HCEA source and PDB metadata;
   it is not a role invented to alter allocation.
2. Replace the inherited `boolean *` member-stride loop and pointer-to-`long`
   comparison in `player_ui_clear_multiplayer_joins` with one ordinary indexed
   `for` loop over the real local-player array. The first formulation is exact;
   there was no expression, lifetime, or declaration-order search.
3. Use the existing `_game_connection_local` and
   `SAVED_GAME_FILE_TYPE_PLAYER_PROFILE` constants, and `TEST_FLAG` with the
   existing `_saved_game_file_default_profile_bit` instead of manual shifting.
4. Store the multiplayer variant as the existing owning
   `struct game_variant`, not `byte[0x68]`; the two `csmemcpy` calls now take its
   address and `sizeof(*variant)`. `game/game_engine.h` already supplies the
   type and its 0x68-byte size assertion. The January stores, copies, following
   field offsets, and the 816-byte BSS extent remain unchanged. No shared type
   was modified or moved.

Whole-source inspection finds no assembly, `volatile`, `register`, forced
inline/noinline, pragma, optimizer barrier, fake lifetime, byte emission,
consumer-local external prototype, or raw tag/object getter cast. Public APIs
have declarations in their included owner headers. Void bodies explicitly
return and parameters use the vertical project style. The lexical fake-match
scan reports zero leads; that scanner does not resolve the remaining layout
question below.

## State-name and linkage evidence

January file offset `0x2FD5A4` / decimal `3134884` is a four-byte initialized
`NONE` datum. Its only incoming relocations in `config/relocs.json` are:

| Relocation file offset | Caller |
| --- | --- |
| `0xD016A`, `0xD019C` | `player_ui_remember_player1_profile` |
| `0xD01EA`, `0xD01F0` | `player_ui_get_player1_last_used_profile_index` |

Read-only donor Git `scratch/halocea-full-audit` at
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains those same two functions in
`src/blam/interface/`; both call the state `player1_last_used_profile_index`.
The direct PDB command was:

```powershell
& 'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe' -sym player1_last_used_profile_index 'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb'
```

It reports `File Static`, `Type: long`, `DataKind: 0x5`, RVA `0x146C704`.
The January extracted public-symbol inventory does not name this private
datum. Thus static linkage is strongly triangulated from the later explicit
PDB record and January's confined call graph, **not claimed as a surviving
January private-record observation**.

Root's exact pending map change is:

```json
{"file_offset":3134884,"flags":0,"name":"_player1_last_used_profile_index","static":true}
```

Scratch gates below use the in-memory diagnostic source alias
`player1_last_used_profile_index=data_002fd5a4` only because the current split
still has the old label. Neither production source nor flags contain that
alias. The renamed scratch owner has static storage class 3; the old January
split still has producer-default class 2 until root updates the map.

## Complete owner inventory

All 42 target code owners compare strict exact against the final aliased
scratch object. There are no candidate-only code owners. All 25 target runtime
non-code sections compare strict exact: one 816-byte BSS, one four-byte DATA,
and 23 RDATA sections totaling 1,141 logical bytes. Reported RDATA contribution
extents include alignment and total 1,184; reported total data is 2,004 bytes.
Do not confuse logical byte lengths with padded progress extents.

The public `player_ui_globals` anchor is at BSS offset zero in both objects,
storage class 2, size 816, zero relocations, normalized SHA-256
`0645a4a67dcec462dc9f335bb0564e6e39bf12ea7e40cf8de81418210102c2d1`.
The private last-profile state remains at DATA offset zero, size four, zero
relocations, normalized SHA-256
`ad95131bc0b799c0b1af477fb14fcf26a6a9f76079e48bf090acb7e8367bfd0e`.
There are no new writable owners or COMMON allocations.

The final candidate has exactly three additional RDATA strings, totaling 64
logical bytes. Each is naturally used by real source, has COMDAT selection 2
(`select any`), and compares strict exact to an existing canonical base copy:

| String owner | Bytes | Existing canonical base copy | January selected owner |
| --- | ---: | --- | --- |
| `??_C@_00CNPNBAHC@?$AA@` | 1 | `source/game/game_engine.obj` | `source/ai/action_obey.obj`, file `0x24340F` |
| `??_C@_0DN@PDKODJDH@?$CIcontroller_index?$DO?$DN0?$CJ?5?$CG?$CG?5?$CIcontro@` | 61 | `source/input/input_abstraction.obj` | same unit, file `0x26F474` |
| `??_C@_11LOCGONAA@?$AA?$AA@` | 2 | `source/game/game_engine.obj` | same unit, file `0x25B198` |

All three January contributions also record selection 2. The current partial
`action_obey` base does not emit its January-selected empty-string owner, but
the canonical game-engine copy is present and identical. The string hashes,
in table order, are
`6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d`,
`b279940c75c29e11fcd6f19aa9e07694eced150a118543c947f36ea8ff43de85`, and
`96a296d224f285c67bee93c30f8a309157f0daa35dc5b87e410b78630a09cfc7`.
This proves compatible existing copies, not a successful final linked image.

## Link-test boundary — not a pass

A normal XDK `link.exe` diagnostic attempted `/dll /noentry /nodefaultlib
/opt:ref /machine:x86`, the four real objects (final aliased Player UI,
canonical Action Obey, Input Abstraction, and Game Engine), and `/include:`
roots for the three strings. It failed with `LNK1120: 163 unresolved
externals` from the incomplete engine dependency graph. The requested
`scratch/player-ui-string-coalescing-20260904.map` is zero bytes; no DLL was
produced. No `/force` option, stub owner, synthesized function, or patched COFF
was used to turn this failure into a claimed success. The experiment does
not establish actual link coalescing or whole-program correctness.

## Remaining source boundary and reopen condition

The public `playlist_profile` is still only forward-declared in its owner
header, while Player UI carries a local partial view with an opaque
`unused1D[0x47]` middle. Those bytes are real variant state, not established
unused padding. The current January-facing view proves the used name/flags
positions and 0x68-byte span, but it is not evidence for a complete authentic
original playlist definition. The later HCEA game variant is 152 bytes and
must not be transplanted. Simply renaming the opaque bytes would not solve
that source-recovery problem. No speculative layout or header edit was made.

Keep the object `NonMatching` while this public schema/ownership question and
the requested actual coalescing proof remain open. A future bounded family
packet should authenticate the January playlist definition, place its proper
public layout in its owner without dependent exact regressions, and establish
the required ordinary link evidence. The 42 already-exact functions should
remain credited independently of this whole-object boundary.

## Reproduction and frozen artifacts

For each stage the command was:

```text
python tools/campaign/gate.py source/interface/player_ui --alias player1_last_used_profile_index=data_002fd5a4 --forbid-emitted-symbol _point_from_line3d --out scratch/<artifact>.obj
```

The baseline omits `--alias`. Each stage reports 42 exact / 0 residual /
0 unwritten and an explicit emitted-symbol guard pass.

| Artifact under `scratch/` | Raw SHA-256 |
| --- | --- |
| `player-ui-owner-baseline-20260904.obj` | `e623ed6a41a2e0c812c1fd35e27c67cbc7e221ba25959f14835939086937a491` |
| `player-ui-owner-renamed-oldalias-20260904.obj` | `3357cac72bbf6efab9528e0076fa07824087448c7281cfb8ab395c5ae72b42bc` |
| `player-ui-owner-typed-joins-20260904.obj` | `eb4b56ac6bb2e268a5ce113aea03d7721322e043699376ff598f6e5a41dc66c1` |
| `player-ui-owner-enum-flags-20260904.obj` | `b474e9853c0f68bbcc72ddb669c70c76263e7e839530a7756b26aabbf3b3d168` |
| `player-ui-owner-typed-variant-20260904.obj` | `8138696a9ad34d1e289a889e22539dbcea630fc038f4381821fb351bf5f6c30a` |

Frozen physical source SHA-256:
`f57dbcb19e13d4e9d160f226aa04ae20ea1bb2c9a5c613138bab3854f689b6c2`.
Git line-ending normalization may change that physical hash at commit time.
`git diff --check` is clean. The scoped fake-match scan is zero leads.

The existing hidden-exact accounting candidate is
`_set_local_player_controls_from_player_profile`: 528 meaningful/padded bytes,
14 relocations, normalized SHA-256
`ff47c65c6eda0c31b625817bb914f77ad790244d2be44109e484d10664685ee5`.
Its raw objdiff score is 95.89041%; the hardened comparator is exact. Adding
an independently checked semantic entry would correct reporting of existing
progress, not add new reconstructed bytes. No manifest was edited here.
