# `playlist_profile.obj` default-data owner closeout (2026-09-13)

## Verdict

`source/saved games/playlist_profile.obj` is ownership-complete and is admitted
as `Matching`.  The first natural reconstruction of
`_playlist_profile_default_data` is exact: one packed, typed aggregate owns the
26 default-variant builders and the trailing `first_time` Boolean.  The
PID-isolated hardened gate remains **14 exact / 0 residual / 0 unwritten**,
and an isolated ordinary objdiff report now accounts for **2,637 / 2,637 code
bytes** and **1,137 / 1,137 target data bytes**.  No fuzzy or candidate-only
section receives target credit.

`config/config.json` therefore marks this unit `Matching` for the
orchestrator's global regeneration and verification pass.

## Natural source owner

The translation unit now defines
`struct playlist_profile_data playlist_profile_default_data`.  The structure
is packed to the measured 0x69-byte extent and has compile-time checks for the
0x68-byte pointer array, the `first_time` offset 0x68, and total size 0x69.
`source/game/game_engine_playlist.h` is the genuine narrow associated owner for
the builder API and has this translation unit as its sole consumer, so the 25
missing declarations were added there beside the existing Slayer declaration.
Every parameter remains on its own line.

The initializer order is:

```text
slayer, slayer_pro, elimination, phantoms, endurance, rockets, snipers,
oddball, reverse_tag, accumulation, juggernaut, stalker, king, king_pro,
crazy_king, race, rally, ctf, invasion, iron_ctf, ctf_pro, team_race,
team_rally, team_oddball, team_king, team_slayer; TRUE
```

This is one genuine owner, not a byte array, linker trick, or split array and
Boolean facade.  The latter would introduce a second symbol/section owner, and
ordinary unpacked layout would be 0x6C rather than January's 0x69; both were
rejected from the measured layout before compilation.  No alternate codegen
shape was needed.

## Exact COFF evidence

The January owner and the rebuilt owner agree on every recorded property:

| property | January | rebuilt |
| --- | --- | --- |
| section / logical size | `.data` / 105 | `.data` / 105 |
| flags | `0xC0400040` | `0xC0400040` |
| owner | `_playlist_profile_default_data` | same |
| value / type / storage | `0 / 0 / external (2)` | same |
| raw logical form | 104 relocation-slot zero bytes, then `01` | same |
| relocations | 26 `IMAGE_REL_I386_DIR32` | same |
| normalized SHA-256 | `bca1e941486d011882aea09506432903b0be136249bac89dd9b9844ca830d687` | same |

Relocations occur at `0x00, 0x04, ..., 0x64`, in exactly the initializer order
above.  Each relocation type, destination symbol identity, and addend agrees;
`coff_compare.section_infos_equal` is true.  The existing 116-byte
`_playlist_profile_globals` BSS also remains exact with flags `0xC0400080`,
external ownership at zero, no relocations, and the same logical-zero payload.
All 916 bytes of January-owned read-only/string data remain ordinary exact.

The direct defined-runtime-owner census has no target-only owner.  Target and
candidate share all 21 non-code external owners.  The candidate has only the
following compiler/header-emitted duplicate COMDATs, all outside target credit:

| candidate-only owner | bytes | incoming relocations | normalized SHA-256 | rebuilt canonical provider |
| --- | ---: | ---: | --- | --- |
| `??_C@_05DFJCHPDH@input?$AA@` | 6 | 1 | `3a289232399a207509f4fa9d70fa6f68817ce6a66f2150ff082336311f4e55fe` | `source/bungie_net/network/transport_endpoint_winsock.obj` |
| `??_C@_07IFGLIMJI@variant?$AA@` | 8 | 4 | `d1a8855b4331858b62ec8f8a0dfb637a6ab57f1434f713f83258afc7ca228b95` | `source/interface/player_ui.obj` |
| `??_C@_0CK@DLIFJMDN@ui?2default_multiplayer_game_sett@` | 42 | 1 | `69d08adb631b7b9607a00354752495dec50ad176d9251e6b82f111da6a15e688` | `source/interface/ui_widget_group.obj` |
| `??_C@_0CO@BDMJPAMG@metadata?5name?5may?5not?5match?5game@` | 46 | 1 | `cc922c912f92ddd0b76987bbf77696c76dbeb60114dfec79405e637c1db77cc8` | `source/saved games/player_profile.obj` |
| `??_C@_0FA@LJKGPDPC@failed?5to?5get?5saved?5game?5files?5m@` | 80 | 2 | `e3d96d32eb348bd39258d7545e7acca6fa3ac54bc8ee6f560c982982b63fdb1f` | `source/saved games/player_profile.obj` |
| `_D3DTEXTUREDIRECTENCODE` | 16 | 0 | `2df64c1861ed539052d35303c159771b5caeaaed5bfa04b343eb47d48329f807` | `source/cseries/headers.obj` |
| `_D3DSIMPLERENDERSTATEENCODE` | 328 | 0 | `e29e444c4976f669d2d4606f5795ee3a6c65d326bbdf8312b31dc735487075f3` | `source/interface/progress_bar.obj` |
| `_D3DPRIMITIVETOVERTEXCOUNT` | 88 | 0 | `45dcae937d7b20c7c6c2bacbe90deabbbaa9311f99cd26e5387a6d2a21de1404` | `source/rasterizer/xbox/rasterizer_xbox_decals.obj` |

Every row is external/value-zero/type-zero, has zero internal relocations,
uses `IMAGE_COMDAT_SELECT_ANY` (selection 2), and is strict section-equal to
the rebuilt provider including flags.  `config/symbols.json` and
`config/contribs.json` independently identify the January linked providers.
Diagnostic XDK links of the unmodified candidate plus all seven rebuilt
providers, in both input orders, exit zero under `/FORCE:UNRESOLVED` with
**zero `LNK2005` and zero `LNK1169`**.  Only expected unresolved-symbol warnings
remain.  This proves compatible ordinary-link coalescing; the 614 duplicate
bytes are neither target-owned data nor new credit.

## Provenance and evidence limits

The January split object is byte authority.  The January PDB's playlist module
is module 33 (124 bytes) and contains only OBJNAME (`0x0009`) and COMPILE
(`0x1013`) records, so it contributes no private type or data-symbol record.
Its public-symbol corpus does authenticate all 26
`_build_game_variant_*` names, but not `_playlist_profile_default_data`.
The owner name comes from the existing January address recovery at
`0x003168C8`.

As cross-build corroboration only, the HCEA reconstruction
`src/data/default_variant_building_functions.c` contains 38 builders and its
first 26 names are exactly January's relocation order with the same
`struct game_variant *fn(struct game_variant *)` signature.  No user-supplied
playlist source was found; the supplied-source reconstruction map covers other
families.  Neither HCEA nor an older donor object was treated as January byte
authority.

## Exact / fuzzy / rejected disposition

- **Exact:** the 105-byte initialized owner, its 26 ordered relocations, all
  116 BSS bytes, all 916 target read-only bytes, and all 14 functions.
- **Fuzzy:** none.
- **Rejected:** unpacked 0x6C layout and split-owner facades, for measured
  layout/ownership reasons above.  Candidate-only SELECT_ANY duplicates are
  recorded and link-proved but explicitly rejected from target credit.
- **Unresolved:** none inside this object.  The `struct playlist_profile`
  source-layout rejection in `config/object_admission_rejections.json` belongs
  to `source/interface/player_ui`, not this unit.

## Verification

```text
python tools/campaign/gate.py "source/saved games/playlist_profile" --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/playlist-profile-default-data-owner.obj
```

reports the 14 exact padded sections as 448, 48, 96, 64, 304, 336, 16, 624,
80, 160, 352, 96, 64, and 32 bytes; zero residual and zero unwritten; the
forbidden-symbol guard passes.

The isolated one-unit objdiff report marks the unit complete with `.text`
2,720, `.rdata` 916, `.data` 105, and `.bss` 116 all at 100%.  Direct admission
classification reports zero candidates, contradictions, or rejections.
`tools/fake_match_scan.py` scans the C file and owning header with **zero review
leads**.  JSON parsing and `git diff --check` pass.

Candidate object SHA-256:
`d29eff60d8b02855dbf6467917591ba74ab1a4fa542117acdbc3325649bb37c4`.
January split-object SHA-256:
`7662910ff6b95330e936749a6e873c6b1c64eff3c5c645181698bd1e725bb267`.
The source SHA-256 at verification is
`57c710fcd5f3f5c2ad61e869ad3e8d5b1c6869c8739c625689bc1e140b3fde00`.

No global configure, Ninja build, commit, or push was run in this closeout.
