# `playlist_profile.obj` save-wrapper first-shot boundary

## Result

The first and only natural production candidate for
`source/saved games/playlist_profile.obj::_playlist_profile_save` preserved
the January size, control flow, calls, and relocation addresses, but did not
match the target's register allocation or complete symbolic ownership packet.
The candidate was removed completely. This additive record grants no code,
data, semantic, object-completion, or admission credit.

| Packet | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| January target | 71 | 80 | 6 | `22dc5b74a7bef3ad2d1964e74abe3f94d2d21c4420cf1c2c1c2d4bb743a86095` |
| Sole candidate | 71 | 80 | 6 | `67e0e54dbe294186fa9341190f45cf2d5013f078513f06b640ecf0164fc2fc6a` |

Both packets assert the variant at source line `0x131`, reject the `NONE`
profile index, clean the variant, call `_code_001b2240` with the index, and
return. They share all six relocation addresses and call destinations:
source path at `+0x14`, `variant` literal at `+0x19`, `_display_assert` at
`+0x1E`, `_system_exit` at `+0x25`, `_game_engine_variant_cleanup` at
`+0x36`, and `_code_001b2240` at `+0x3C`.

January keeps the variant in ESI and defers saving EDI until after the null
test; the current VC7.1 translation unit saves both registers first, keeps the
variant in EDI, and keeps the profile index in ESI. The target also references
the shared `variant` string owner symbol, while the incomplete candidate
translation unit emits that literal as a locally defined non-code owner. A
strict comparison therefore fails even though the wrapper has the same
80-byte envelope and semantic operations.

## Evidence and sole candidate

January COFF is authoritative. The HCEA `playlist_profile_save.c` donor
independently confirms the `NONE` guard, in-place variant cleanup, and handoff
to the write path. January additionally proves the pre-guard assertion, exact
line, helper identity, and ordinary cdecl ABI. The current typed
`game_engine_variant_cleanup` declaration supplies the grounded
`struct game_variant *` parameter. The existing Playlist Profile ledgers,
repository history, current source, and available donor trees contained no
earlier save-wrapper body or measured save-wrapper boundary.

The sole candidate was:

```c
void playlist_profile_save(
	long playlist_profile_index,
	struct game_variant *variant)
{
	match_assert(
		"c:\\halo\\SOURCE\\saved games\\playlist_profile.c",
		0x131,
		variant);
	if (playlist_profile_index != NONE)
	{
		game_engine_variant_cleanup(variant);
		code_001b2240(playlist_profile_index);
	}

	return;
}
```

The generated XDK 3911 edge compiled once. Direct hardened comparison then
reported the equal meaningful/padded sizes and relocation positions above,
but unequal normalized bytes and the `variant` owner-identity difference. No
local copy, parameter reorder, const variant, declaration variant, lifetime
rewrite, register-shaping expression, source placement experiment, compiler
option, or second candidate was compiled.

The rejected source used no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, forced inline, barrier, raw address or offset access,
pointer/integer reconstruction, pun, undefined behavior, byte forcing,
comparator exception, or tool exception. Reopening requires genuinely new
source-order, prototype, or cross-build evidence rather than a spelling
sweep.

## Scope

The source file was restored byte-for-byte. The inherited exact Playlist
Profile set had no loss during the sole candidate comparison and is unchanged
after restoration. All protected/excluded objects and concurrent lanes remain
untouched; the cumulative gate rechecks the protected Units sentinel.
