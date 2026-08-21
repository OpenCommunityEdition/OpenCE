# `player_profile.obj` Jonas wrapper pair (2026-08-21)

## Result and fixed boundary

This Jonas-owned wave starts from exact canonical commit
`6de91d1d95a71515d03cac7785cda00175c0233c` on isolated branch
`jonas/player-profile-wrapper-quartet-20260821`. The dispatched boundary named
four public bodies. January-PC authentication failed closed on
`_player_profile_save` before any source edit or emission because its private
helper uses an ESI-plus-stack calling convention. The remaining three ordinary
cdecl bodies were present together in the first and only natural
code-producing candidate compile. The enumerate wrapper and delete leaf were
independently strict immediately and are retained. The ARGB accessor missed
and was removed wholesale without a spelling, declaration, control-flow, or
compiler-control retry.

The final source-only pair advances `player_profile.obj` from 5/22 to 7/22
exact functions and from 192/4,119 to 267/4,119 meaningful code bytes. It
remains `NonMatching`. The strict net gain is 75 meaningful bytes, 80 padded
bytes, and four exact relocations. No data credit is claimed.

| Fixed candidate | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 | Decision |
| --- | ---: | ---: | ---: | --- | --- |
| `_player_profiles_enumerate_available_to_local_player_index` | `0x001B0840` | `31 / 32` | 1 | `f6ace3d32df95a8dc3d144fa0ed8ce370d5ad42c12267e7c58c33f2beb2f682f` | **strict exact, retained** |
| `_player_profile_delete` | `0x001B0860` | `44 / 48` | 3 | `b099e605e5fd63851afdaa68d6526ee3a02ac42f3428698ad84750109fb00846` | **strict exact, retained** |
| `_player_profile_get_argb_color` | `0x001B1440` | `68 / 80` | 1 | `733b6e189ab86d153c588b799a279fc40047a862d19155e00ce0dce17a3094c3` | first natural candidate missed; removed |
| `_player_profile_save` | `0x001B16B0` | `63 / 64` | 5 | `1750c6c90e87691d51f4b301076ddb0c3670662e67cc997fbfe55d2a4998e8ba` | private ABI; excluded before compile |

## Immutable target and source provenance

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/saved games/player_profile.obj`: 17,152 bytes,
  SHA-256
  `22c5fc38c3b1090f8e0ef271ce2119fa57d8c5c8aafe65c4947d638fc96d6c57`.
- Baseline `source/saved games/player_profile.c` Git blob:
  `ae8a596eb3f5b42324287f885edb546153cfd010`, 5,562 payload bytes,
  with Git-blob payload SHA-256
  `80261f50ab6d495ef1fe1644386193189db63044c96b674b2fdc3b3571d45da2`.
- Retained translation-unit Git blob:
  `5d665339daa0d606e34c43e80b9c9f2fefe79d52`, 6,607 payload bytes,
  with SHA-256 of the exact staged Git-blob payload
  `77b0e12f2c91b0ef79abd6c3d51e3d9be56d3e7e53bea5303a3379c5672909da`.
  These are Git payload bytes, not CRLF working-tree bytes.
- The shared `source/saved games/player_profile.h` is unchanged at blob
  `210ea1b1597fc03e339a27fdc31714c99a7c3921`, 825 payload bytes, payload
  SHA-256
  `1a738ac888a14c89aa68115fdf542bb638b216029896cfbff6cf5aa475697618`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
  the natural flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 include graph.

The clean HCEA semantic reference is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. All four cited exact-name files
are directly tree-reachable there. Their identities below were re-read from
that clean worktree; hashes are over exact `git cat-file blob` payloads.

| HCEA path | Git blob | Bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| `src/player_profiles_enumerate_available_to_local_player_index.c` | `9147128bf53e15b879a9911289da14c359ef97b3` | 920 | `8ca6d60b479ea37b18943154a88dac9bd73e3a40065b26554b28ab76049150ff` |
| `src/player_profile_delete.c` | `37372076c0d9f9ff34c9da6dfb58f2b67e02657a` | 218 | `a4148baf04fc025dc7bfd882330dc6768d3e02067dd7f3c1372d94ed84c6860b` |
| `src/player_profile_get_argb_color.c` | `988cda35bf3b79c795684a0138898356965963fc` | 448 | `b1ffca52d1afb0468c6dd79631365534c5ae0501010971978234cd9f1e11976a` |
| `src/player_profile_save.c` | `414603b29843122298d164113f5296db7286af6a` | 647 | `0f39da88cacda055c42abaacb3e177d71fbbabcc1b7f7e79cf47252c27f4841d` |

HCEA is labeled semantic provenance only. January is authoritative for PC
signatures, calls, assertions, literals, error behavior, code extent,
relocations, and acceptance. In particular, HCEA's delete donor discards the
boolean result and omits January's error path, while HCEA's save donor calls a
public-looking writer and omits January's assertion/private-helper contract.
No Stian source or donor layout is used in this wave.

All-revision history searches find no prior committed definition of any of
the four dispatched bodies in this owner TU. January defines each public
symbol once. The fixed scope did not collide with an integrated or active
player-profile wave.

## Typed ABI and caller boundary

The retained ordinary-C contracts are:

```c
void player_profiles_enumerate_available_to_local_player_index(
	short local_player_index,
	word *number_of_profiles,
	long *player_profile_indices,
	boolean include_default_profiles);
void saved_game_files_enumerate_available_to_local_player_index(
	short local_player_index,
	word saved_game_file_type,
	word *number_of_profiles,
	long *player_profile_indices,
	boolean include_default_profiles);

void player_profile_delete(long player_profile_index);
boolean delete_enumerated_saved_game_file(long saved_game_file_index);
```

`short` and `word` are 16-bit, `long` is 32-bit, and `boolean` is an 8-bit
value promoted naturally at the cdecl call boundary. The source identifiers
decorate as `_player_profiles_enumerate_available_to_local_player_index`,
`_saved_game_files_enumerate_available_to_local_player_index`,
`_player_profile_delete`, and `_delete_enumerated_saved_game_file`; the final
COFF proves those exact public/dependency symbols without a cast or alias.

The complete reconstructed-source caller census finds one non-owner TU,
`source/interface/ui_widget_event_handler_functions.c`. It declares and calls
the enumerate wrapper three times and calls delete once. Its older local
enumerate declaration spells the count pointer `long *`, but all downstream
uses explicitly consume the low `word`, and pointer representation/call ABI is
unchanged. HCEA's exact-name contract and the saved-game-layer call fix the
callee's actual count storage as `word *`. No shared-header edit is needed or
retained, and no storage/data-owner declaration changes.

Delete uses the existing typed `error(short, const char *, ...)` interface and
the authenticated `_error_silent == 2` priority. January's exact compiler
literal is:

```text
player_profile_delete() failed (profile index= #0x%lX)
```

The body skips `NONE`, calls the boolean delete helper for every other index,
and emits that error only on false. The literal's 55-byte COMDAT independently
equals January with normalized SHA-256
`171869f7b36de699a038e73577f23ea28ab74a6899d904b1da916a2597a27b2d`.
It is compiler-owned support for the code proof and receives zero standalone
data credit.

## Save precompile exclusion: hidden ESI ABI

January fixes the apparent public signature as
`void player_profile_save(long player_profile_index, player_profile *profile)`
and asserts non-null `profile` at source line `0x100`. The profile pointer is
loaded from `[ebp+0x0C]` into ESI. For a non-default index the caller pushes
only the index from `[ebp+0x08]` and calls `_code_001b15f0`, leaving the
profile solely in ESI.

The helper independently confirms that contract: at RVA `0x001B15F3` it
begins `test esi, esi`; later it pushes ESI as the source of a 0x30-byte copy,
while the only formal stack argument is read from `[ebp+0x08]`. A second
January caller, `_player_profile_save_level_completed`, executes
`lea esi, [ebp-0x30]`, pushes the profile index, and calls the same helper.
Therefore `_code_001b15f0` is not ordinary cdecl C and cannot be honestly
declared or called from this wave. No helper declaration, cast, wrapper, raw
overlay, or save body was added, and save is recorded as an authenticated
precompile exclusion rather than a compiled miss.

## Strict relocation proof and inherited bodies

The hardened comparator proves the retained enumerate relocation at `+0x16`
is `REL32` to
`_saved_game_files_enumerate_available_to_local_player_index`.

Delete's three ordered relocations are independently identical:

| Offset | COFF type | Destination |
| ---: | --- | --- |
| `+0x0E` | `REL32` | `_delete_enumerated_saved_game_file` |
| `+0x1B` | `DIR32` | exact delete-error literal COMDAT |
| `+0x22` | `REL32` | `_error` |

All five inherited accepted bodies remain independently strict:
`_player_profile_number_of_available_primary_colors`,
`_player_profile_get_rgb_color`,
`_player_profile_get_enclosing_directory_path`,
`_player_profile_get_random_good_color`, and
`_player_profile_get_random_color`. Direct hardened comparison proves the
complete seven-body inventory has identical padded bytes, normalized hashes,
and relocation semantics.

## One-shot ARGB rejection and storage boundary

The natural typed ARGB donor shape used a local `real_rgb_color`, called the
already exact `player_profile_get_rgb_color`, assigned alpha 1.0 and copied
the three RGB fields. Its sole code-producing candidate emitted 64 padded
bytes with one relocation at `+0x0F` and normalized SHA-256
`76ea4a0de2f296a971a776d266aa6827fcdd1f4c0f7ee46650ebc3103e1a16eb`.
January requires 68 meaningful / 80 padded bytes, one relocation at `+0x12`,
and target hash
`733b6e189ab86d153c588b799a279fc40047a862d19155e00ce0dce17a3094c3`.
The body was removed immediately. No expression-order, declaration,
qualifier, local-layout, or compiler-switch retry occurred. Reopen it only
with independently authenticated January-PC source evidence that naturally
explains the longer schedule.

The target object owns 72 `.data` bytes in
`_player_profile_primary_colors`, 108 unmatched `.bss` bytes in
`_player_profile_globals`, and 1,464 `.rdata` bytes. The exact primary-color
array and its existing 72 data bytes predate this wave and remain unchanged.
The final base object defines no `.bss` or COMMON symbol and does not define or
reference `_player_profile_globals`; the new delete literal is the only new
runtime non-code COMDAT. Matched data therefore remains 72/1,644 and this wave
claims zero data.

The retained implementation is readable typed defined C. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, object-representation cast, pointer or union pun,
inactive-union access, undefined signed overflow, private/register ABI,
volatile scheduling device, force-inline annotation, optimizer pragma or
barrier, synthetic anchor, or byte-forcing expression.

## Scope and validation

Only `source/saved games/player_profile.c` and this new Jonas-owned ledger are
tracked changes. No header, configuration, semantic exception, parked record,
completion label, storage owner, pre-existing Markdown, or tracked deletion is
changed. The five protected sources remain untouched:
`source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, `source/ai/actions.c`, and
`source/units/units.c`.

- Complete `halobetacache_build` plus `libcmt_build`: pass.
- Direct hardened COFF comparison: both retained and all five inherited
  accepted functions are strict; rejected ARGB and precluded save are absent.
- Semantic audit: 470 units, 4,133 functions evaluated, 3,993 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,054 accepted exact, 36
  ordinary-only, 35 structurally accepted, one rejected, and zero unit errors.
- Campaign progress: 375/833 matched objects, 4,041/11,060 exact functions,
  481,006/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo progress: 273/468 matched objects, 3,874/7,574 exact functions,
  468,092/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes. Libcmt remains 102/212 objects,
  167/476 functions, 12,914/55,015 code bytes, and 5,192/8,637 data bytes.
- `player_profile.obj`: 7/22 exact functions and 267/4,119 meaningful code
  bytes; it remains nonmatching and gains no data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check`, source scope,
  protected-five, banned-construct, and deleted-path checks are clean.

A clean committed-state snapshot followed by verified same-path deletion,
normal Ninja rebuilding, regression checking, and direct seven-body comparison
is the final reproducibility proof. No push, amend, or history rewrite is
performed.

## Clean committed-state replay

Implementation-and-ledger commit
`89ee4bd1895b644ddc0e014b5698e53056ec18fc` was clean before replay. Its
committed source blob is exactly
`5d665339daa0d606e34c43e80b9c9f2fefe79d52`; re-reading the Git object
produced 6,607 bytes and the recorded payload SHA-256
`77b0e12f2c91b0ef79abd6c3d51e3d9be56d3e7e53bea5303a3379c5672909da`.
The shared header remained baseline blob
`210ea1b1597fc03e339a27fdc31714c99a7c3921`.

A one-unit regression snapshot was captured directly from that clean commit.
Because the generated Ninja edge correctly escapes the directory space as
`saved$ games` while the current regression metadata parser looks for the
decoded base path, snapshot/check used an ignored one-edge parser view with
the exact generated compiler command and cflags. Both operations used
`--no-build`; the actual rebuild below used the unmodified generated
`build.ninja` and its genuine production edge. No configuration or regression
tool was edited.

The resolved generated path
`build/base/source/saved games/player_profile.obj` was verified inside this
isolated worktree, deleted, confirmed absent, and rebuilt through its normal
same-path Ninja target. The rebuild executed exactly one `[1/1] CL` action
with the repository's natural flags and no source change or retry.

The immediate regression check passed with all seven accepted functions
`still_exact`, `changed_nonexact: []`, `newly_exact: []`, and zero failures or
warnings. Direct hardened January comparison again returned
`all_equal: true` for the complete seven-body inventory. In particular it
reproduced enumerate at 31 meaningful / 32 padded bytes, one relocation, and
normalized SHA-256
`f6ace3d32df95a8dc3d144fa0ed8ce370d5ad42c12267e7c58c33f2beb2f682f`,
and delete at 44 meaningful / 48 padded bytes, three relocations, and
normalized SHA-256
`b099e605e5fd63851afdaa68d6526ee3a02ac42f3428698ad84750109fb00846`.
The rebuilt object is 3,054 bytes with phase-specific whole-file SHA-256
`e503b1468ba5dc614cb83f95da4ff4e03a4d0b470629d7d6dd2bf5b3be6b2375`.

The rejected `_player_profile_get_argb_color`, precluded
`_player_profile_save`, and private `_code_001b15f0` definitions are absent
from the rebuilt object. It defines no `.bss` or COMMON symbol, retains only
the pre-existing exact 72-byte primary-color `.data`, and adds only the exact
delete-error compiler COMDAT with zero standalone data credit. This section
is an additive ledger-only follow-up to the implementation commit.
