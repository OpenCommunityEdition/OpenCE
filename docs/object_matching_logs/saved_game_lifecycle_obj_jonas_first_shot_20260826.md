# Saved-game lifecycle first-shot package (2026-08-26)

## Result

This Jonas-owned wave starts from clean cumulative commit
`86977d76d4b775a9882c55ce4022966a6030720e`. Four evidence-closed lifecycle
bodies were frozen together and emitted in the first and only ordinary
code-producing candidate compile. All four are independently strict exact:

| Function | Owner object | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_player_profiles_dispose` | `player_profile.obj` | 93 / 96 | 10 | `3fc1e9da57f3b007c36ee4fed3d96c04fcdb50e588fa7333df8b88d7d292cac0` |
| `_playlist_profiles_initialize` | `playlist_profile.obj` | 25 / 32 | 3 | `3cf05cb816e4666cf6d5fe5294a184a46b273c5e2869e0b65d420b636423206e` |
| `_playlist_profiles_dispose` | `playlist_profile.obj` | 93 / 96 | 10 | `7b160fbc39dd25fc1e458d83abacd5cf1a548cb3b9f1fdb298428216ae1d9953` |
| `_saved_game_files_dispose` | `saved_game_files.obj` | 74 / 80 | 9 | `8849150b357a6fbb25db012ea381df3a61dd43f9eb49566d662a7818403f385b` |

The strict gain is four functions, 285 meaningful bytes, 304 padded bytes,
and 32 exact relocations. None of the three objects is complete, so all remain
`NonMatching`. No data or BSS credit is claimed.

Expected progress after the normal report is regenerated is:

- `player_profile.obj`: 8/22 functions and 360/4,119 meaningful code bytes;
- `playlist_profile.obj`: 4/14 functions and 169/2,637 meaningful code bytes;
- `saved_game_files.obj`: 5/46 functions and 111/12,068 meaningful code bytes;
- campaign: 4,084/11,060 ordinary exact functions and
  452,224/2,198,102 meaningful code bytes, with complete objects unchanged at
  377/833 and data unchanged at 1,811,278/4,176,062.

## Immutable evidence boundary

The complete precompile evidence packet is preserved untracked at
`build/audit/saved-game-lifecycle-20260826/FROZEN_PACKET.md`. Its source,
target, and baseline copies were sealed before the ordinary compile.

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1.
- `xbox/bin/vc7/CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Natural flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the unchanged XDK
  3911 and repository include graph.

| Object | January split SHA-256 | Pre-wave base SHA-256 | First candidate SHA-256 |
| --- | --- | --- | --- |
| `player_profile.obj` | `22c5fc38c3b1090f8e0ef271ce2119fa57d8c5c8aafe65c4947d638fc96d6c57` | `86efa26e5186f95e4532751a2262e071ce25f52a3b6d3c71a99a4b44d703873c` | `76f20e93c1a252fbc8df0cff995323090ec673d170e964c946d151f5c84538eb` |
| `playlist_profile.obj` | `9b257ee9ef620c8d2cedc267a19019dc4b2259a0ad7e54117199008d010df6db` | `0071903c424198a46380b417c54a2551256228fc124f6331b6ccf14ddb41e95a` | `c84f4496c28ad41354c81025c1fdbd17db38b30e3cedcffbf6762263f8c28be1` |
| `saved_game_files.obj` | `bd000ef1323c25d9bba631cd0c1a0ef9c6f3bbca7bcc5a634fc9ec13bc8fedf4` | `d43116abc66bd99b39fcf2ba689a0d9b4637fb6dae1756675ad86bb6f4cee214` | `6f0a505dc46a3b01b41127e68a850a88cb3954fddbda93a49a61507f198d9d8b` |

The raw candidate hashes include phase-specific COFF debug records. Admission
uses the hardened section comparator and exact ownership inventory, not raw
whole-object equality.

The frozen source payloads are:

| Translation unit | Base Git blob | Candidate Git-blob identity | Working-tree SHA-256 |
| --- | --- | --- | --- |
| `source/saved games/player_profile.c` | `5d665339daa0d606e34c43e80b9c9f2fefe79d52` | `857d8325eb62913fb850da5fb4e8d34ba7300db8` | `0d986a62aeb62938387b21e72c603aea73d8a6f0d174dbdc47e8adff77358370` |
| `source/saved games/playlist_profile.c` | `a2709b06c04e6fc062fadcf73c512d213643ac04` | `300e27d2c477d4dea55aabc9728012141fd00f93` | `6a0c649198ce7d1287335c85ca48e13ac356b44942e20f92c9a30da2e45c4e42` |
| `source/saved games/saved_game_files.c` | `385312669fdcf6e9e9e2af13a799a33530e39c51` | `85e64c1e2eaeafadad03ccc7eb14cc50d5960487` | `ce0b37b0417e2c64f51c528bf0f899adb1c51853f026f18d938dc316cf5639a0` |

All three source packets passed VC7.1 `/Zs` under the natural flags before
the fixed candidate compile. No source byte changed between that seal and
emission.

## Binary-authenticated layouts and behavior

All writable storage remains externally owned. The local structures expose
only the January offsets needed by these bodies and carry compile-time offset
and size checks.

- `player_profile_globals`: size 0x6C, thread pointer +0x64, initialized byte
  +0x68.
- `bss_004d2858`: size 0x74, thread pointer +0x6C, default-profile count
  +0x70, initialized byte +0x72.
- `saved_game_files_globals`: size 0x11C, general mutex +0x10C, mapfile mutex
  +0x110, initialized byte +0x116.

`playlist_profiles_initialize` clears exactly 0x74 bytes through `csmemset`,
sets the initialized byte true, and returns.

Both profile disposers use the same January topology: test the thread, emit
the silent waiting error once when it exists, spin until `thread_has_exited`,
dispose and null the thread, then clear the entire globals structure. The
exact January literals are:

```text
waiting for asynchronous player profile writes to finish...
waiting for asynchronous playlist profile writes to finish...
```

Their complete COMDAT sections are independently exact:

| Literal | Bytes | SHA-256 |
| --- | ---: | --- |
| player wait | 60 | `9b1124a3119fe4b23944349a82efac473b372bfc672b1778779b48a69276086e` |
| playlist wait | 62 | `50f4da17335311c207360149e7501c1584e404832bae6846675e434a9b9d697a` |

`saved_game_files_dispose` tests, disposes, and nulls the general mutex,
repeats that sequence for the mapfile mutex, calls player disposal then
playlist disposal, clears initialized, and returns. The exact relocation
sequence proves both mutex offsets and the cross-object dependency order.

## Relocation proof

The hardened comparator proves ordered relocation address, COFF type,
destination owner, and addend.

`_playlist_profiles_initialize`:

| Offset | Type | Destination |
| ---: | --- | --- |
| +0x05 | DIR32 | `_bss_004d2858+0` |
| +0x0A | REL32 | `_csmemset` |
| +0x13 | DIR32 | `_bss_004d2858+0x72` |

Each profile disposer has four thread-owner DIR32 references, one exact
waiting-string DIR32, REL32 calls to `_error`, `_thread_has_exited`,
`_dispose_thread`, and `_csmemset`, plus the final globals-base DIR32. The
player addends are +0x64 and the playlist addends are +0x6C.

`_saved_game_files_dispose`:

| Offset | Type | Destination |
| ---: | --- | --- |
| +0x01 | DIR32 | `_saved_game_files_globals+0x10C` |
| +0x0B | REL32 | `_dispose_mutex` |
| +0x14 | DIR32 | `_saved_game_files_globals+0x10C` |
| +0x1D | DIR32 | `_saved_game_files_globals+0x110` |
| +0x27 | REL32 | `_dispose_mutex` |
| +0x30 | DIR32 | `_saved_game_files_globals+0x110` |
| +0x39 | REL32 | `_player_profiles_dispose` |
| +0x3E | REL32 | `_playlist_profiles_dispose` |
| +0x44 | DIR32 | `_saved_game_files_globals+0x116` |

## Inherited exact inventory and ownership

Direct hardened comparison proves all previously accepted functions still
strict in the same first artifacts.

Player profile inherited inventory:

- `_player_profiles_enumerate_available_to_local_player_index`;
- `_player_profile_delete`;
- `_player_profile_number_of_available_primary_colors`;
- `_player_profile_get_rgb_color`;
- `_player_profile_get_enclosing_directory_path`;
- `_player_profile_get_random_good_color`;
- `_player_profile_get_random_color`.

Playlist profile inherited inventory:

- `_playlist_profile_delete`;
- `_playlist_profile_number_of_default_profiles_on_disk`.

Saved-game inherited inventory:

- `_saved_game_file_get_type`;
- `_saved_game_files_notify_memory_units_changed`;
- `_saved_game_files_release_mutex`;
- `_enumerate_memory_units_test`.

The player object's pre-existing 72-byte
`_player_profile_primary_colors` `.data` remains byte exact with normalized
SHA-256
`1a359e1b4ea25af0b08dfe89b383fdc3ee2b2dca14d60d5aa8860059cee9e89f`.
The playlist and saved-game candidates define no runtime writable section.
All three objects have no COMMON symbols. `_player_profile_globals`,
`_bss_004d2858`, and `_saved_game_files_globals` are zero-valued undefined
external symbols in the candidates. Thus this wave neither claims nor moves
BSS/data ownership.

## Provenance and version control

The clean HCEA semantic donor is commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Exact-name donor files are under
`research-cache/halocea-cseries-20260820/src/blam/saved_games/`, with layouts
under the adjacent `src/headers/` tree. Stian independently corroborates the
saved-game disposal order at
`research-cache/stian-halo-cseries-20260820/src/halo/saved games/saved_game_files.c`.

Those sources are semantic evidence only. HCEA is a later PowerPC build whose
profile layouts are much larger and whose two profile disposers omit
January's mandatory `_error_silent` calls. Stian's raw addresses and function
casts are likewise inadmissible. January PC bytes, calls, offsets, strings,
scope, and relocation ownership are authoritative.

The following Claude policy documents were also incorporated:

- `C:/halo-worktrees/claude-ai-conversation-2/AGENTS.md`;
- `C:/halo-worktrees/claude-ai-conversation-2/CLAUDE.md`;
- `C:/halo-worktrees/claude-ai-conversation-2/docs/verification_policy.md`;
- `C:/halo-worktrees/claude-ai-conversation-2/docs/references/prototype-inference.md`;
- `C:/halo-worktrees/claude-ai-conversation-2/docs/references/kb-update-policy.md`.

No rich later-build structures, KB edits, raw addresses, static-scope changes,
or public-header refactors are imported.

## Scope and source rules

Only the three owner translation units and this ledger are tracked changes.
No shared header, configuration, semantic exception, parked record, status,
data owner, or protected Units/AI path changes.

The retained source is C89 and uses typed local structures and ordinary
prototypes. Every no-argument function uses `void` on its own line, every
function has an explicit final `return;`, and the source introduces no
assembly, raw address, pointer/integer reconstruction, object representation
cast, union pun, undefined behavior, volatile scheduling device, optimizer
pragma, barrier, force-inline spelling, or code-forcing expression.

Before commit, `git diff --check` is clean. The first candidate objects and
the complete frozen evidence packet remain preserved under
`build/audit/saved-game-lifecycle-20260826/`. A clean committed-state replay,
normal cumulative build/report, semantic audit, whole-TU regression check,
tool tests, and object-admission/parked gates follow after integration. No
push, amend, or history rewrite is performed.
