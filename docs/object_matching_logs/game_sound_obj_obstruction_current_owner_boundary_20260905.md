# Game Sound obstruction: current-context ownership boundary (2026-09-05)

## Disposition

**WITHHELD.** The current natural source reconstructs the last unwritten
Game Sound function and preserves all27 exact functions and the three existing
residuals, but emits an additional48-byte `vector_from_points3d` COMDAT.
Ordinary XDK linking against the real canonical selected owner fails with
LNK2005 in both input orders. Nothing was applied to production; no park,
exact-code/data credit, or whole-object claim is made.

January is498 meaningful/512 padded bytes with33 relocations, normalized SHA
`2040bfc2f77a485e4461abbeaa0b724c9d53feba4169d45f605c53ff3fd29dc5`.
The one current natural candidate is496 padded bytes with33 relocations, SHA
`7e4a9f3f5ca4a088a990d7cc277c4bd83417aa8d58774f48ba3a5f4cc36bfcb5`.
It is not exact. No ordinary fuzzy percentage is claimed here: an interactive
CLI diff invocation was canceled, and a percentage is unnecessary to establish
the demonstrated ownership veto.

## Correction of the remembered PAS blocker

The older PAS header rejection applied to `game_sound_update(real dt)`, not
`compute_sound_obstruction`. The blocked packet explicitly left obstruction
unwritten/out of scope. The PAS header issue was later resolved and integrated
as documented in
`game_sound_obj_pas_current_context_integration_20260904.md`.

The20260828 public-leaf ledger described obstruction as a large collision/PVS
routine with global/math ownership sensitivity; it did not establish an
obstruction-specific current header failure. Current canonical already has:

- the correct three-argument public prototype in `game_sound.h`;
- the genuine SoundManager caller;
- complete0x40-byte sound_source and observer/location/collision layouts used
  by existing implementations;
- collision-user globals and enums from the existing collision_usage header;
- the real structure-BSP distance and PVS APIs.

The duplicated full sound_source definition is inherited shared-type cleanup
debt, not a new prefix or altered ABI introduced by this packet. No type,
header, foreign prototype, symbol metadata, storage owner, or active-Fable
implementation was changed.
This is not a waiver of the rule that shared API/layout types belong in their
owner header. That ownership cleanup remains unresolved too, and must be
coordinated with the active Fable SoundManager work rather than silently
accepted because the current complete layouts are compatible.

## One natural source packet

The read-only frozen Opus donor is
`C:/halo-worktrees/opus-small-families-30k-20260902/source/sound/game_sound.c`,
with its `scratch/reports/game_sound.md`. Its historical512/512 residual and
register-pinning discussion are not current admission evidence and were not
replayed as an exactness search.

The current candidate preserves the donor's complete January behavior while
using the existing named cluster-distance mask and the ordinary existing
`vector_from_points3d` API. This source-level normalization was made before
the sole candidate compilation, not chosen after measuring competing forms.
The direct component-subtraction donor was not compiled as an alternate to
suppress the helper. No manual-inline or source-shape retry followed failure.

January disassembly independently proves the observer call, collision-user
push/pop and three assertions (882/887/926), pessimistic obstruction/occlusion
stores, cluster and PVS gates, collision flags0xC0E1, and final occlusion ratio
and PIN clamp. HCEA `src/compute_sound_obstruction.c` corroborates semantics
only; its different collision flag names and omitted assertions were not
copied. Project `real`, enums, bit-vector macros and explicit return are used.

The complete research-only body is preserved here for future reconciliation:

```c
void compute_sound_obstruction(
	short local_player_index,
	struct sound_source *source,
	real distance)
{
	struct observer_result const *camera = observer_get_camera(local_player_index);

	match_assert(
		"c:\\halo\\SOURCE\\sound\\game_sound.c",
		882,
		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
	global_current_collision_users[global_current_collision_user_depth++] =
		_collision_user_sounds;

	match_assert(
		"c:\\halo\\SOURCE\\sound\\game_sound.c",
		887,
		source->spatialization_mode==_sound_spatialization_mode_absolute);
	source->obstruction = 0.6f;
	source->occlusion = 1.f;

	if (source->location.game_location.cluster_index != NONE
		&& camera->location.cluster_index != NONE)
	{
		real cluster_distance =
			(structure_bsp_get_cluster_encoded_sound_distance(
				global_structure_bsp_get(),
				camera->location.cluster_index,
				source->location.game_location.cluster_index) & CLUSTER_SOUND_DISTANCE_VALUE_MASK)
			* (MAXIMUM_CLUSTER_SOUND_DISTANCE / CLUSTER_SOUND_DISTANCE_VALUE_MASK);

		if (cluster_distance < MAXIMUM_CLUSTER_SOUND_DISTANCE)
		{
			if (BIT_VECTOR_TEST_FLAG(
				structure_bsp_get_cluster_pvs(
					global_structure_bsp_get(),
					camera->location.cluster_index),
				source->location.game_location.cluster_index))
			{
				real_vector3d vector;
				struct collision_result collision;

				source->obstruction = 0.45f;
				vector_from_points3d(
					&camera->position,
					&source->location.position,
					&vector);
				if (!collision_test_vector(
					FLAG(_collision_test_front_facing_surfaces_bit) |
						FLAG(_collision_test_structure_bit) |
						FLAG(_collision_test_media_bit) |
						FLAG(_collision_test_objects_bit) |
						FLAG(_collision_test_objects_scenery_bit) |
						FLAG(_collision_test_objects_machines_bit),
					&camera->position,
					&vector,
					NONE,
					&collision))
				{
					source->obstruction = 0.f;
					source->occlusion = 0.f;
				}
			}

			if (source->obstruction != 0.f)
			{
				source->occlusion = 1.f - distance / (cluster_distance + distance);
				source->occlusion = PIN(source->occlusion * 1.4f, 0.f, 1.f);
			}
		}
	}

	match_assert(
		"c:\\halo\\SOURCE\\sound\\game_sound.c",
		926,
		global_current_collision_user_depth > 1);
	--global_current_collision_user_depth;

	return;
}
```

## Ownership evidence

The saved-object census changes30code/24noncode/3COMMON to32/28/3.
Every inherited code and non-code owner retains bytes, ordered relocations,
symbol storage/type/offset, section flags and selection. The unchanged COMMON
owners are game_sound_globals4, game_looping_sound_data4, and combined_pas64.
The only new code is obstruction plus the unwanted vector helper. No
point_from_line3d is emitted.

The four new non-code owners are the genuine spatialization assertion, two
pooled collision-user assertions, and the1.4 real constant. No data credit is
claimed and their final all-owner admission remains unnecessary once the
ordinary code-owner conflict is established.

A complete census scanned833 configured January targets and572 built bases
with no parse errors. Both January and actual canonical select
`source/ai/action_alert.obj` as the no-duplicates (selection1) vector owner.
Candidate helper and selected owner have identical48-byte normalized code:
`905f56dbe4597d2bacc201456ffc33464b166e0b6392dda24a1a3e109b637aa2`,
zero relocations, external storage2/type32, alignment16, flags0x60501020.
The candidate's selection2 is the incompatible difference.

Unmodified candidate and frozen actual ActionAlert base were linked in both
orders using XDK Link.Exe with /NOLOGO /DLL /NOENTRY /NODEFAULTLIB.
Both report `LNK2005: _vector_from_points3d already defined`, followed by57
expected unrelated unresolved externals (LNK1120). Neither produces a DLL.
No /FORCE, /IGNORE, synthetic stubs, object patch, compiler flag change or
linker accommodation is used. This is a demonstrated duplicate-owner failure,
not a ban on all natural extra helpers merely because they exist.

No full-game or bounded emulator behavior test is claimed for this withheld
packet. A source/control-flow review and passing function regression gate do
not override the link failure.

## Frozen evidence

| Artifact | SHA-256 |
| --- | --- |
| `scratch/game-sound-obstruction-target-frozen-20260905.obj` | `1b78edd8695556049f6b8728981251ce47a894c5b60eeb0d7776ab04463bbe39` |
| `scratch/game-sound-obstruction-canonical-before-20260905.obj` | `90246b3b2be6dafa4dc8d275eeb47562572fbb27bb1896077691bbbea1c580fe` |
| `scratch/game-sound-obstruction-current-natural-20260905.obj` | `7a86c7291dad9fbc43d8dbe8179f0163b2071167934f8313dc100987345ac11c` |
| `scratch/game_sound_obstruction_current_natural_20260905.inc` | `6eea55fe9e81c98f838e3075866ead8d631d007afafff63d6dbffd949ae6d85c` |
| `scratch/game_sound_obstruction_current_owner_audit_20260905.json` | `1b6d00aa22b409d9468ef4e3a736f2a106ca1c6d2548c18d7ef58923692270af` |
| `scratch/game_sound_obstruction_pooled_owner_audit_20260905.json` | `630812f60c0899db19e6431be427656ee30f0c1ad551a8bf5bfc8737a2cf7900` |
| `scratch/game_sound_obstruction_vector_link_audit_20260905.json` | `a1d0ad82e872fb434fa34e38f517aa8d310ae27b1f357b7032cc7eb53bfab393` |

The adjacent audit scripts reproduce the censuses and ordinary link tests.
The minimal source edit manifest is
`scratch/game_sound_obstruction_current_natural_20260905.edits.json`.

Reopen only with authentic original source/header inline-schedule evidence or
a demonstrated ordinary whole-owner resolution. Do not use manual expansion,
forced inlining, helper suppression, metadata changes, header-position
permutations, or the donor's register-pinning discussion as a workaround.
Canonical remains27exact/3residual/1unwritten, unchanged.
