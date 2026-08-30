# `playlist_profile.obj` Jonas available-enumeration first shot

## Result

`_playlist_profiles_enumerate_available_to_local_player_index` matched in the
first and only production candidate compile. The natural readable typed C89
body is retained.

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_playlist_profiles_enumerate_available_to_local_player_index` | 50 | 64 | 4 | `cc9bc6850acd46fcaa19abf0ee9fc44ebd0e05d2024b3a372784658d2f3f4e21` |

The ordered relocations are DIR32 `+0x05` and `+0x13` to
`_data_003168c8+0x68`, REL32 `+0x0D` to `_code_001b1c10`, and REL32 `+0x29`
to `_saved_game_files_enumerate_available_to_local_player_index`.

## Evidence and documentation audit

January's split object is authoritative. It proves an ordinary cdecl public
wrapper with three stack arguments. It compares the byte at the target-owned
data composite's `+0x68` member with `TRUE`; on first use it calls the private
default-profile creator and clears the byte. It then forwards the local-player
index, playlist file-type value `1`, count pointer, index buffer, and `TRUE` to
the public saved-game enumerator.

The HCEA research cache at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains semantic donor blob
`eb68326f33c870e976a69cde6aecb6b668cb69ed`, physical SHA-256
`0dadd4a757edd73f016cb7a2e31ecfc35d700d49ac8fdc55ca618d0ba8096cd0`.
It independently corroborates the lazy first-use initialization and the five
enumerator arguments. January corrects the donor's speculative 32-bit
`first_time_0` declaration to the directly observed byte member and preserves
the target's private helper name.

The existing exact `player_profiles_enumerate_available_to_local_player_index`
homolog authenticates the saved-game enumerator's typed cdecl prototype and
the same push schedule. The current UI caller independently declares the
playlist wrapper with `short`, `word *`, and `long *` parameters and supplies
all three arguments.

The prior Playlist Profile ledger was read completely. It documents the two
retained public leaves, one rejected display-name attempt, data-ownership
rules, the generated-Ninja escaped-space caveat, and the applicable Claude
and toolchain provenance. The current Claude `AGENTS.md`, `CLAUDE.md`,
prototype-inference, ABI/calling-convention, verification-policy, snapshot,
and verification-explained documents were also reviewed. Their binary-first,
C89, narrow-prototype, and strict validation rules were applied; later-build
semantics and donor tooling were not allowed to override January COFF.

Canonical history, object ledgers, registered worktrees, and current source
contained no earlier production definition or candidate for this function.

## Retained source and ownership

The pre-shot source blob was
`300e27d2c477d4dea55aabc9728012141fd00f93`; the retained blob is
`7874b42e6df3a7bb52ecd9780ecff78ae236c78c`.

The source adds a translation-unit-only prefix type with compile-time proofs
that `first_time` is at `0x68` and the recovered prefix size is `0x69`. The
target-owned `_data_003168c8` remains an undefined external; the candidate
defines no writable data, BSS, COMMON, or aggregate owner. The retained body
is:

```c
void playlist_profiles_enumerate_available_to_local_player_index(
	short local_player_index,
	word *number_of_profiles,
	long *playlist_profile_indices)
{
	if (data_003168c8.first_time == TRUE)
	{
		code_001b1c10();
		data_003168c8.first_time = FALSE;
	}

	saved_game_files_enumerate_available_to_local_player_index(
		local_player_index,
		_saved_game_file_type_play_list,
		number_of_profiles,
		playlist_profile_indices,
		TRUE);

	return;
}
```

There is no assembly, `volatile`, `register`, pragma, intrinsic, attribute,
compiler control, barrier, raw address or offset access, pointer/integer
reconstruction, pun, undefined behavior, byte forcing, or comparator
exception.

## First-shot and validation evidence

The pre-shot rebuilt object SHA-256 was
`1d35cc6a73a9374f42cc36d3f8ce4991de48fffe5d78f499ec54ce422c48fc26`.
After the baseline artifact was preserved and the literal candidate path was
proved absent, the ordinary generated edge ran exactly once:

```text
[1/1] CL "build\base\source\saved games\playlist_profile.obj"
```

The 2,727-byte first artifact has SHA-256
`cacdf0be0b317837b83842cf6c1732c875b868b70eadd74cd2b422585b4952fc`.
Direct hardened comparison reports `all_equal: true` for the complete
64-byte section and all four ordered relocation records. Its disassembly is
byte-for-byte January, including fourteen trailing NOP padding bytes.

Whole-object exact-set comparison reports this function as the sole
`newly_exact` owner and an empty `lost_exact` list. The inherited exact
Playlist Profile functions remain exact. The generated gate's known inability
to parse the escaped `saved$ games` edge is not bypassed or treated as
evidence; the direct comparator and exact-set audit operate on the resolved
production object paths.

The protected Units sentinel, full build/reports, admission/park audits, and
tooling-test replay are recorded at the cumulative integrating head after the
concurrent lanes are merged.
