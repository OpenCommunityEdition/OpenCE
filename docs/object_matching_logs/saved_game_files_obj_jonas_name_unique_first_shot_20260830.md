# `saved_game_files.obj` unique-name first-shot boundary

## Result

The first and only natural production candidate for
`source/saved games/saved_game_files.obj::_saved_game_file_name_unique`
reproduced the January stack allocation, arguments, calls, padded extent, and
return semantics, but not its byte-boolean local topology. The candidate was
removed completely. This additive record grants no code, data, semantic,
object-completion, or admission credit.

| Packet | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| January target | 81 | 96 | 3 | `5d3602ef5c358e7eadd8255e81613e25a4f849bb631965e19d22d6fbc5ea84c6` |
| Sole candidate | 81 | 96 | 3 | `8d722c521abbd38e24b3a8152678428b96e93c14edd4182878280af1828caa6e` |

January and the candidate both reject a null or empty wide name, allocate an
eight-byte ASCII root and 256-byte save-game path, convert the target-owned
wide save root with `_wide_to_ascii`, and call stock XDK
`XCreateSaveGame(root, name, OPEN_EXISTING, 0, path, 256)`. A nonzero XDK
result means the requested name is available/unique.

January saves EBX and seeds `BL=FALSE` before the input guards. Its final
truth selection uses `TEST EAX; MOV AL,1; JNE; MOV AL,BL`. The natural
early-return candidate needs no saved byte local, moves its three relocations
three bytes earlier, and lowers the final comparison through
`NEG/SBB/NEG EAX`. Thus the target relocations are at `+0x2B`, `+0x37`, and
`+0x40`, while the candidate's identical destinations occur at `+0x28`,
`+0x34`, and `+0x3D`.

## Evidence and sole candidate

January COFF is authoritative. The target-owned eight-byte
`_data_00316934` packet is a typed pair of pointers: a wide `u:\\` save root
and the independent `z:\\saved\\hdmu.map` path. Only the first member is read
here. Its target data owner has two DIR32 relocations and is not claimed by
this rejected code lane. The stock XDK declaration authenticates the six
stdcall parameters and `DWORD` result. The Pastudan readable lift
independently confirms the null/empty guards, buffer extents, conversion, and
nonzero-result meaning. HCEA's later-build body is an always-false stub and is
not January authority.

Repository history, current source, all Saved Game Files ledgers, and
available donor trees contained no earlier candidate or measured boundary for
this function. The sole readable typed C89 candidate used the ordinary XDK
and Unicode declarations:

```c
boolean saved_game_file_name_unique(
	wchar_t const *name)
{
	char ascii_root[8];
	char save_game_path[256];

	if (name && *name)
	{
		return XCreateSaveGame(
			wide_to_ascii(
				data_00316934.saved_game_root,
				ascii_root,
				sizeof(ascii_root)),
			name,
			OPEN_EXISTING,
			0,
			save_game_path,
			sizeof(save_game_path)) != ERROR_SUCCESS;
	}

	return FALSE;
}
```

The generated XDK 3911 edge compiled once. Direct hardened comparison found
the equal meaningful/padded extent but the relocation shifts and boolean
topology above. No explicit boolean local, assignment topology, ternary,
guard regrouping, signature variant, const variant, buffer rewrite, compiler
option, or second candidate was compiled.

The rejected source used no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, forced inline, barrier, raw address or offset access,
pointer/integer reconstruction, pun, undefined behavior, byte forcing,
comparator exception, or tool exception. Reopening requires genuinely new
source-topology evidence rather than a spelling sweep.

## Scope

The owner source was restored byte-for-byte. The inherited exact Saved Game
Files set had no loss in the sole candidate comparison and remains unchanged.
All protected/excluded objects and concurrent lanes are untouched; the
cumulative gate rechecks the protected Units sentinel.
