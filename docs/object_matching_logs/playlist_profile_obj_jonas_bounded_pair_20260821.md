# `playlist_profile.obj` Jonas bounded pair

## Result

This bounded source-only wave starts from authoritative integration commit
`63c0252e3d088d52daaa59f45d2320555dac790f`. The two fixed public
ordinary-cdecl bodies were emitted together in exactly one natural
code-producing candidate compile. `playlist_profile_delete` was independently
strict. `playlist_profile_get_display_name` missed and was removed wholesale
with its support-only declaration and include; it was not retried or tuned.

`playlist_profile.obj` advances from 1/14 to 2/14 exact functions and remains
`NonMatching`.

| Retained function | RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_playlist_profile_delete` | `0x001B1A60` | 44 / 48 | 3 | `b099e605e5fd63851afdaa68d6526ee3a02ac42f3428698ad84750109fb00846` |

The inherited `_playlist_profile_number_of_default_profiles_on_disk` remains
strict at 7/16 bytes, one relocation, and normalized SHA-256
`1f82cf78b0df84e5b0bf619acb9ebfc8c165ff2558679479ef137123faa560a2`.
The hardened comparator proves identical padded bytes and identical relocation
address, type, destination, and addend for both accepted functions.

## Provenance and Git payload authentication

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `playlist_profile.obj`: 13,728 bytes, SHA-256
  `9b257ee9ef620c8d2cedc267a19019dc4b2259a0ad7e54117199008d010df6db`.
- Final accepted candidate object before commit: 1,296 bytes, SHA-256
  `dc34b7c3b8f4255ff1682f92f329a4c2fa2f98f49752119592618d36d757c5bd`.
- One-shot two-body candidate object SHA-256:
  `38627252ae09dbfbed92cd598db28cb703b7296ddf25bde9bf8404749563924e`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Clean HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`.

Every Git identity below was read from its raw blob payload after the retained
source was staged through Git's clean filter. The byte count and SHA-256 thus
authenticate the payload independently of the Git object ID.

| Evidence | Git blob | Bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| baseline `source/saved games/playlist_profile.c` | `eb54644aee6c857a116cffb87df0b0709a4ec6a2` | 3,081 | `929a1e0ffda9ced143fd573d973d403c3ff0fb9f4c200cf6c59b59d3c05db7c6` |
| retained `source/saved games/playlist_profile.c` | `a2709b06c04e6fc062fadcf73c512d213643ac04` | 3,483 | `faa645db6d3a039b4332eb3fede3f216ef6c7af5010621f08d9c288d8fde0161` |
| canonical exact homolog `source/saved games/player_profile.c` | `5d665339daa0d606e34c43e80b9c9f2fefe79d52` | 6,607 | `77b0e12f2c91b0ef79abd6c3d51e3d9be56d3e7e53bea5303a3379c5672909da` |
| canonical `source/cseries/errors.h` | `81f2b8e018d21cb5853e55d181992c3700e49a30` | 1,128 | `5304821730b3c268676d1d23c1966f37b29432569bd0c255f6e5de642f68cc79` |
| canonical `source/cseries/cseries.h` | `3c91ac46ab275894a18ac4e839b38ae13022d91b` | 10,070 | `e994e965cd3f48cd47cebf0a39f41a1aac460e8b25544452a1fc4b302a0b7043` |
| canonical UI caller/declaration TU | `c0af6f6d425958500e02672a73f4608771801ad2` | 207,676 | `2bb6cea45ce598e6224dbab66ff68549f07a1b9c0c6c2d52a5a4d73c55c13654` |
| HCEA delete donor | `ad242a6a8c938941007481a34388ccb6223d5379` | 226 | `60d405c6eae30f1497bb1fe2a0de167ef14ae1482e4a27bbd9980b83eea60ea7` |
| HCEA screened display donor | `96f5fc070c58a23c2f837ea5bf05409176322e47` | 506 | `1e5a05934258558e75c04c38e0bfb52bd6539853da0052c512a93ed3caa66531` |

The c168 worktree is clean and both supplied blobs are reachable from that
exact tree. Canonical history has no earlier body attempt for either fixed
function. Prior commits in this TU concern an explicitly withdrawn owned-BSS
experiment and the retained extern-only default-count leaf. No existing
playlist-profile branch, worktree body, or matching ledger collides with this
wave.

## Signature, caller, and relocation proof

The screened signatures were:

```c
void playlist_profile_delete(
	long playlist_profile_index);
boolean playlist_profile_get_display_name(
	long playlist_profile_index,
	wchar_t *display_name); /* screened, rejected */
```

January has one direct delete call at RVA `0x000DF3A3`, inside
`_code_000df390`. The canonical caller declares a `long`, pushes the full
32-bit profile index, performs caller stack cleanup, ignores a result, and
continues with its own byte-boolean return. The target delete callee reads the
ordinary `[ebp+8]` stack slot and returns with plain `ret`. HCEA independently
uses a 32-bit index and the `NONE` guard. This authenticates the ordinary cdecl
signature and C identifier that emits `_playlist_profile_delete`.

The target delete body rejects `NONE`, calls the byte-boolean
`delete_enumerated_saved_game_file`, and reports a false result at
`_error_silent` priority with the exact message
`playlist_profile_delete() failed (profile index= #0x%lX)`. Its machine code
is identical to the current strict `player_profile_delete` homolog except for
the relocated message owner. The homolog's natural typed source therefore
authenticates the retained spelling independently of the one-shot result.

The three exact delete relocations are REL32
`_delete_enumerated_saved_game_file` at `+14`, DIR32 canonical message literal
at `+27`, and REL32 `_error` at `+34`.

The screened display target reads both full 32-bit stack slots, returns a byte
boolean in `AL`, and returns with plain cdecl `ret`. Its two relocations are
REL32 `_saved_game_file_get_display_name` at `+8` and REL32 `_ustrncpy` at
`+28`. The success path passes exactly `0x7F` wide characters and writes a
16-bit NUL at output byte offset `0xFE`, proving element index 127. January has
no direct or address-taken caller for this unused public leaf; the target ABI,
canonical `boolean`/`wchar_t` widths, and HCEA semantic donor govern its
screening.

Every TU-local dependency uses the undecorated C identifier that emits the
target leading-underscore COFF spelling. No stdcall suffix, private helper, or
register ABI enters this wave.

## One-shot rejection and retained source

The display candidate preserved the target 53/64 extent and both relocation
destinations, but its normalized SHA-256 was
`165267bf5bcb37a344ea419e21d0216c6db70d587b390a871e0b3d5157c86974`
instead of target
`b52d949230e98323168a50c0af6cc57ed4d7367094548c78700b5ee11687ce90`.
Its early-false topology emitted `_ustrncpy` at `+32`, four bytes after the
target `+28` relocation. The body, getter declaration, and support-only
Unicode include are absent from final source. There was no alternate branch
topology, declaration, local lifetime, literal, or scheduling attempt.

The retained body is readable typed C: it checks `NONE`, calls the typed
boolean deletion service, reports a false result using the canonical error
API, and ends with an explicit terminal return. The existing typed BSS prefix,
compile-time offset proof, extern declaration, and exact default-count body
are byte-for-byte unchanged.

Before the one code-producing boundary, the January split was generated by
the already authenticated local csplit executable and the final source passed
VC7 `/Zs` parse-only type checking; the candidate object was verified absent.
Neither prerequisite emitted candidate code. The two-body TU was then compiled
once. All later compilation is the normal final accepted-set/full-build or
committed-state replay required by validation, never a candidate retry.

The source contains no assembly, raw address or offset, pointer cast or
overlay, type pun, inactive-union read, undefined behavior, volatile
scheduling device, register forcing, optimizer pragma or barrier, synthetic
anchor, or byte-forcing expression.

## Ownership and scope boundary

Only `source/saved games/playlist_profile.c` and this new Jonas ledger change.
Initialization, disposal, creation, path lookup, display-name retrieval,
enumeration, profile access, save, every private helper, and all shared headers
remain untouched. No configuration, semantic exception, parked record,
completion label, pre-existing Markdown, protected-five source, tracked-path
deletion, or owned BSS/data change is in scope.

The final candidate object defines exactly the retained delete and inherited
default-count code COMDATs plus the 57-byte canonical failure-message literal.
It defines no writable `.data`, `.bss`, COMMON symbol, or aggregate owner.
`_bss_004d2858`, `_delete_enumerated_saved_game_file`, and `_error` remain
undefined externals. The rejected display symbol is absent. The literal is
required support for accepted code but receives no data-matching credit; all
1,137 target non-code bytes remain unclaimed.

## Validation before commit

- One natural two-body candidate compile: one strict accept and one
  fail-closed wholesale removal, with no candidate retry.
- Complete Halo and libcmt builds plus generated ordinary and semantic
  reports: pass. Inherited warnings remain confined to unrelated translation
  units; `playlist_profile.c` emitted no warning.
- Direct hardened final comparison: retained delete and inherited default
  count exact; rejected display absent; no writable or COMMON ownership.
- `playlist_profile.obj`: 2/14 functions, 51/2,637 meaningful code bytes, and
  zero data credit; remains `NonMatching`.
- Semantic audit: 470 units, 4,141 functions evaluated, 4,001 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,062 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,049/11,060 exact functions,
  481,554/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 3,882/7,574 functions and
  468,640/1,770,166 code bytes; libcmt contributes 167/476 functions and
  12,914/55,015 code bytes.
- Admission: zero candidates and zero revocations; the one inherited
  `shell_xbox` completion-label contradiction remains explicit.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, source-policy, deleted-path, protected-scope, and
  changed-path audits: pass.

The implementation commit made no advance claim about committed-state replay.
The additive record below supplies the actual clean snapshot, verified object
deletion, normal Ninja rebuild, regression check, direct comparator,
rejected-symbol absence, and final ownership.

## Recorded committed-state replay

The implementation commit is
`674085d43eaffd6f46bc81df9e51cbcc9bb78f2a`. At that clean committed state,
`source/saved games/playlist_profile.c` resolves to Git blob
`a2709b06c04e6fc062fadcf73c512d213643ac04`; its raw committed payload is
3,483 bytes with SHA-256
`faa645db6d3a039b4332eb3fede3f216ef6c7af5010621f08d9c288d8fde0161`.
`git status --short --branch` reported only the branch header and no changed or
untracked path.

The initial direct regression snapshot invocation stopped before deletion or
building because the current metadata parser looks for the decoded base path
while generated Ninja correctly escapes the directory as `saved$ games`. The
successful snapshot/check therefore used an ignored one-edge parser view with
the decoded metadata path, exact generated compiler command, and byte-for-byte
identical cflags. Both operations used the tool's hidden `--no-build` switch.
The actual rebuild used the untouched production `build.ninja` and its genuine
escaped-path edge; no tracked configuration or regression tool was edited.

The clean snapshot was written to
`build/regression_playlist_profile_delete_20260821.json` with status
`SNAPSHOT_WRITTEN`, commit
`674085d43eaffd6f46bc81df9e51cbcc9bb78f2a`, and sole unit
`source/saved games/playlist_profile`. The generated candidate path was
resolved inside this worktree. Its existing 1,296-byte file, SHA-256
`dc34b7c3b8f4255ff1682f92f329a4c2fa2f98f49752119592618d36d757c5bd`,
was verified before `build/base/source/saved games/playlist_profile.obj` was
deleted; a second existence check proved it absent. The normal repository
Ninja/VC7 edge then rebuilt that exact path with the unchanged compile flags
`/O2 /Oy- /DDEBUG /Dxbox`.

The committed-state regression check returned `ok: true`, no failures, no
warnings, no newly exact functions, and no changed nonexact functions. Its
`still_exact` set is exactly `_playlist_profile_delete` and
`_playlist_profile_number_of_default_profiles_on_disk`. A subsequent direct
hardened comparison again reported strict equality for both code COMDATs,
including every padded byte and relocation address, type, destination, and
addend. The rejected `_playlist_profile_get_display_name` definition is
absent.

The replay object is 1,296 bytes with phase-specific SHA-256
`a493dd396038eee6ce44eb9ae1d25e3fc355996561fc390bdef989f4042daf51`.
Its only defined external code owners are the retained delete and inherited
default-count functions; its only runtime non-code owner is the canonical
57-byte delete-failure literal. There is no writable `.data`, `.bss`, COMMON,
or aggregate owner. `_bss_004d2858`, `_delete_enumerated_saved_game_file`,
and `_error` remain undefined externals. The worktree remained clean after
replay.

Nothing is pushed, amended, or history-rewritten.
