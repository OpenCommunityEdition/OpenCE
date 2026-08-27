# Biped stop-melee recovery (2026-08-27)

## Result

The first and only ordinary code-producing candidate makes
`_biped_stop_melee_attack` strict exact in `source/units/bipeds.obj`. The
object advances from 7/51 to 8/51 exact functions and from 27/24,400 to
57/24,400 meaningful code bytes. It remains `NonMatching`; no data,
completion-label, semantic-exception, or parked-function credit is claimed.

All seven inherited exact functions remain exact:
`_animation_left_foot_frame_index`, `_animation_right_foot_frame_index`, the
four biped lifecycle no-ops, and `_biped_delete`.

## Target, type, and provenance evidence

January defines an external `void(long)` cdecl owner with 30 meaningful and 32
padded bytes. It pushes biped mask `1` and the object index, calls
`_object_try_and_get_and_verify_type`, and, if the returned pointer is non-null,
stores zero to byte offset `+0x45D`. Its sole relocation is a zero-addend
`REL32` at function offset `+0x0A`.

Current typed source independently authenticates every element:

- `biped_try_and_get(index)` in `source/units/bipeds.h` wraps
  `object_try_and_get_and_verify_type(index, _object_mask_biped)`;
- `struct biped_datum` exposes the nested named byte
  `biped.player_melee_ticks` at the target offset;
- the protected Units caller already has the matching house-style
  `void biped_stop_melee_attack(long)` declaration and calls it from the exact
  grenade path.

The complete Claude campaign documents and registered Claude sources were
searched. The readable Claude/Stian reconstruction and the independent HCEA
later-build source agree that this function clears the named melee tick byte
only when the typed biped lookup succeeds. HCEA also corroborates the
`void(int)` public signature and typed biped-mask lookup. Those later builds
are semantic/type provenance only; January PC COFF remains the sole padded-byte
and relocation authority.

No prior `bipeds.obj` production candidate or object ledger implemented this
leaf. Existing Units ledgers mention it only as an external dependency of an
already exact Units caller.

## Retained source and house-rule audit

```c
void biped_stop_melee_attack(
	long biped_index)
{
	struct biped_datum *biped;

	biped = biped_try_and_get(biped_index);
	if (biped)
		biped->biped.player_melee_ticks = 0;

	return;
}
```

The implementation includes the canonical `units/bipeds.h`, uses the typed
object-access macro instead of a raw `object_get` cast, places the parameter on
its own line, keeps the C89 declaration before statements, and ends with an
explicit no-value return. It contains no assembly, `register`, `volatile`,
optimizer barrier, pragma, intrinsic, attribute, raw address, byte-offset
access, pointer/integer reconstruction, pointer or union pun, undefined
behavior, synthetic anchor, object patch, comparator exception, or byte
forcing.

## Immutable artifacts and strict result

- Base commit: `37edb350cf35486aefc2de3bd2e85152bdbb09d9`.
- Pre-wave source blob: `d225ccad2991a62a567ee73811561dd43e334a65`.
- Frozen retained source blob: `33e62b2bbb9db68c714070e1b000636a38cff645`.
- January split object SHA-256:
  `07AF03F457191E94D8D00C0921A056974A9CC4618279FFC09C91C2BCD0BB3721`.
- Baseline candidate object SHA-256:
  `1182B57DF7E0FC2A98F7A3713513CE2A2C87E2A9414DE07956F39A8A7D3A5F5E`.
- First-shot object SHA-256:
  `5C478C399E58BDC740CAB5211F170B906648B2EB2B4D2B20D7695899A9DCB163`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

The hardened comparator proves all 32 padded bytes, relocation count, address,
type, destination identity, addend, and ownership exact. Target and candidate
share normalized SHA-256
`f2f4d0e0fdf61453d808b8bce4aa70f690b21d8e255c57b36556706ce1f10242`.
The exact-set comparison reports only `_biped_stop_melee_attack` as newly
exact, preserves the seven inherited exact functions, and reports no loss.

## One-shot discipline and validation

`git diff --check` and the production compiler's no-output `/Zs` check passed
before emission. Exactly one normal selected-object compiler edge produced the
retained artifact. No type, macro, field, condition, declaration, flag, or
compiler retry occurred.

The complete graph and campaign gates pass:

- 377/833 complete objects overall and 275/468 for Halo;
- 510,820/2,198,102 meaningful code bytes and 4,234/11,060 functions overall;
- 497,906/1,770,166 Halo code bytes and 4,067/7,574 Halo functions;
- data unchanged at 1,836,762/4,176,062 overall;
- semantic audit: 470 units, 4,346 functions evaluated, 4,200 semantic exact,
  4,260 accepted exact, and zero unit errors;
- admission: zero candidates, contradictions, or revocations;
- parks: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

No protected source, configuration entry, completion label, push, amend, or
history rewrite is involved.
