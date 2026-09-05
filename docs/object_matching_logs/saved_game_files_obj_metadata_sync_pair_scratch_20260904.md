# Saved Game Files metadata synchronization pair (scratch, 2026-09-04)

## Scope and authority

This is a scratch-only reconstruction of the real caller/helper pair in
`source/saved games/saved_game_files.obj`:

- `set_nth_entry_in_mapfile`, January 480 padded bytes / 41 relocations;
- `synchronize_metadata_display_name_with_profile_name`, January 1,040
  padded bytes / 63 relocations.

January's split COFF remains the byte and ABI authority.  The existing exact
`get_nth_entry_in_mapfile` body supplies a same-TU source-pattern check for the
write helper.  The HCEX PDB was queried directly with DIA2Dump and independently
reports the public function's parameters as `long index` and
`unsigned short *game_display_name`; HCEA's body is a release stub and is not
used as logic authority.

The January symbol atlas corrects an inherited semantic-name error.  The
private owner at Xbox VA `0x1C5520` is `set_nth_entry_in_mapfile`, not the
current config name `update_nth_entry_in_mapfile`.  The Xbox record is in:

```
research/halo-symbol-atlas/symbols/halo_ce/b49631768ccad84f0b6612227b750e9de8c9af844c9d40e11d76f006ec93dc8f.jsonl
```

The PC cache-beta symbol atlas independently names the corresponding function
at `0x5B3B20`.  The scratch gate therefore used the temporary diagnostic alias
`set_nth_entry_in_mapfile=update_nth_entry_in_mapfile`; production integration
must instead rename the target metadata to `_set_nth_entry_in_mapfile`.

## Authentic APIs and ownership proposal

The source signatures supported by the target ABI are:

```c
static boolean set_nth_entry_in_mapfile(
	word memory_unit_index,
	word n,
	struct enumerated_saved_game_file *file);

boolean synchronize_metadata_display_name_with_profile_name(
	long profile_index,
	wchar_t *game_display_name);
```

The private declaration belongs only in `saved_game_files.c`.  The public
declaration belongs in the existing closest owner,
`source/saved games/saved_game_files.h`; it must not be guessed locally by a
caller.  The exact assertion literal authenticates the local constant spelling
`MAXIMUM_SAVED_GAME_NAME_LENGTH`; the scratch proposal defines it as the XDK
`MAX_GAMENAME` value rather than embedding a magic 128.

Neither function reads or writes `playlist_profile` state or depends on its
schema.  The caller switches only on the existing TU-private
`enumerated_saved_game_file.type`, copies the appropriate `blam.sav` or
`blam.lst`, and uses the already declared game-state persistent-storage
filename API.  No change to the Fable-reserved playlist source or header is
required.

## Reconstruction result

The frozen proposal is:

```
scratch/saved-game-metadata-pair-proposal-20260904.c
SHA-256 89EB9FF4252F45D29C49D5DCE63CEF79C1F0CA40D373A878644CA817B56F6D11
```

It includes the production C file and appends only the two proposed bodies, so
the real private structures, globals, declarations and compiler context are
used without changing production.  The frozen artifact is:

```
scratch/saved-game-metadata-pair-frozen-20260904.obj
SHA-256 21E18440F0EBA950E00AFF2590A899D17B34872BFA370B42AB19776E8FFCA4E6
```

The baseline artifact is
`scratch/saved-game-metadata-pair-baseline-20260904.obj`, SHA-256
`2963912E7EF9C2CFC719475B94BA263056E34F993168AC301F2D494CAEA81E09`.

| Owner | Target | Candidate | Result |
| --- | --- | --- | --- |
| `_set_nth_entry_in_mapfile` (diagnostically compared under the old config name) | 480 / 41 / `32c4cd7f7aecbcf7f45db8fbcb65e2ea00cd63a16bd1c6f63cee05f2ba3cbf2f` | 480 / 41 / same hash | strict exact |
| `_synchronize_metadata_display_name_with_profile_name` | 1,040 / 63 / `d078b8d967245d5c888b930fd2d3048868407c5c14dea8c14f0586cf91749107` | 1,040 / 63 / `8906628ee6e202715d3f0ad9ab72d8e972adeb838f1185fb11cbeeb1b48ff17f` | complete natural fuzzy |

Baseline was **38 exact / 5 residual / 3 unwritten**.  The frozen gate is
**39 exact / 6 residual / 1 unwritten**: all 38 inherited exact owners and all
five inherited residual measurements are unchanged.  The strict gain is 480
padded bytes (474 meaningful bytes through the helper's terminal `ret`).  The
caller reconstructs 1,032 meaningful bytes but receives no strict-byte credit.

The aligned normalized instruction streams for the caller both contain 321
instructions.  After two evidence-backed semantic corrections, the sole code
schedule difference is movement of `mov bl,1` around the two stores of decoded
index fields at function entry.  Zeroing all relocation operands leaves exactly
eight differing raw byte positions, offsets `0x21` through `0x28`; those are the
same eight bytes in the two different orders.  Three `%s%s` relocation lines also display a
whole-object-versus-csplit ownership classification difference while retaining
the same decorated target.  No declaration-order or register-allocation search
was attempted to erase that residual.  Root should generate the required fresh
objdiff percentage before adding a park record.

## Semantic notes

The target's unusual return policy is retained deliberately.  The result starts
TRUE; get/open/mount/XCreate/delete-old/set-mapfile failures log errors but do
not generally clear it.  Only copy failure, a missing `blam.sav` suffix, or an
unknown enumerated file type clears it.  That behavior is directly visible in
January's persistent BL dataflow and is also consistent with HCEA treating the
operation as non-fatal.

The initial draft incorrectly passed `OPEN_EXISTING`; January pushes creation
disposition 1 and the error explicitly anticipates an already-used name, so the
frozen source uses `CREATE_NEW`.  The fourth XCreateSaveGame argument remains
the semantic flags literal `0`.  January emits `push esi` there because the
hard-drive fallthrough proves ESI zero; the literal-zero scratch source emits
that exact instruction.  Passing the decoded memory-unit index as XDK flags
would be nonsensical source and is not authorized by indistinguishable bytes.

The proposal terminates the main renamed path and old persistent path exactly
where January stores terminators.  It does not invent a terminator for the
second persistent `_snprintf` buffer, because January has no such store.  No
opaque padding, representation pun, caller-local API, forced inline, helper
anchor, or inline assembly is used.

## Complete owner boundary

The candidate emits 45 of the 46 January code owners: the 39 exact owners, six
honest residuals, and no candidate-only code.  The only remaining unwritten
owner is the unrelated 992-byte `enumerate_memory_units`.  The exact emitted
symbol guard for `_point_from_line3d` passes.

Relative to the baseline, the pair adds eight selected string COMDATs.  Every
one has an exact decorated January owner: the unknown-type assertion, the
get-entry diagnostic, the saved-name-length assertion, the set-entry failure,
the mapfile-update failure, both XDelete diagnostics, and the XCreate diagnostic.
Other literals are naturally shared with existing functions.  There is no new
COMMON, BSS, aggregate facade, or non-code owner without a January counterpart.

No production C/header/config/park edit, Ninja/configure run, commit, or push was
performed.  Admission remains conditional on root applying the owner prototype
and semantic metadata rename, reproducing the two measurements in production,
refreshing the caller's honest park, and passing the full stable-owner, policy,
data-owner, test and Units sentinel gates.
