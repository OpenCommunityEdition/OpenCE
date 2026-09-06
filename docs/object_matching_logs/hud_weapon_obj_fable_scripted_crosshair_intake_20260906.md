# Fable HUDWeapon scripted-crosshair intake (2026-09-06)

## Verdict

The claimed 48-byte leaf is **not** `_hud_set_crosshair_style`.  The exact
January symbol is `_scripted_hud_show_crosshair` at `0x000C7F10`.  It is a
distinct Fable gain and is not present in the current canonical base object.

The smallest source-coherent packet is conditionally **GO**, owner first:

1. give the TU-local globals pointer the semantic owner
   `weapon_hud_globals` and configure the January datum at file offset
   `4537028` (`0x453AC4`) as `_weapon_hud_globals`, `static: true`;
2. declare `scripted_hud_show_crosshair(boolean show)` in
   `source/interface/hud_weapon.h`;
3. include that owner header from `source/hs/hs.c` and remove only its local
   declaration;
4. prove the owner/API-only packet changes no inherited exact section; then
5. add the unchanged natural `SET_FLAG` leaf in its chronological position
   after the two dispose hooks.

This is a coherent semantic trial, not an exact-match prediction.  The prior
canonical attempt had the right behavior, size, and relocation count but a
register-allocation residual, and current campaign evidence shows that owner
and declaration context can affect other sections.  If the semantic pointer
owner cannot pass the inherited-section gate, **HOLD** the body; do not restore
the raw BSS spelling or search source shapes around it.

No production/config/header edit or compile was performed for this intake.

## January function proof

`build/split/source/interface/hud_weapon.obj` has SHA-256
`760258800d59f9e12750a64cbe63479c7b1ee5a7e577f920328ca46e4fedfbb2`.
Its `_scripted_hud_show_crosshair` section is 48 padded bytes, 43 meaningful
bytes, one `IMAGE_REL_I386_DIR32` relocation at section offset `+0x09`, and
normalized SHA-256
`f106592d3b4916e99cc1f27585794057797128e64f6382b7740801c223f2748f`.
The relocation currently resolves to `.bss:_bss_00453ac4+0` only because the
January datum still has its address-derived config name.

The complete instruction behavior is:

- load the byte-sized `boolean show` argument and test it;
- load the globals pointer through the sole DIR32 relocation;
- load a 32-bit word at pointer offset `+0x1E0`;
- for true, OR bit mask `1`; for false, AND mask `0xFFFFFFFE`;
- store the 32-bit word back at `+0x1E0` and return.

Therefore the accessed member is exactly a 32-bit flags word, not a crosshair
datum, state index, or union.  The natural semantic source domain is the
PDB-named enum `hud_crosshair_show_flags` with
`_hud_crosshair_show_bit = 0`, and the existing house `SET_FLAG` macro expresses
the target behavior without raw masks or offsets.

## Fable claim and chronology

Fable commit `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e` adds the first retained
natural body at `source/interface/hud_weapon.c:178-187`, immediately after
`hud_dispose_weapon_interface`.  It uses:

```c
SET_FLAG(
	weapon_hud_globals->script_flags,
	_hud_crosshair_show_bit,
	show);
```

Its frozen base object SHA-256 is
`f6c10a1d0a384a25577422b9301db6af3fec49aac873e19c954ec0cf43a74046`.
The Fable base and January section are strict equal at 48 bytes, one relocation,
and the normalized hash above.  `scratch/reports/hud_weapon.md:3` records the
starting `5 exact / 0 residual / 11 unwritten` gate; the post-batch facts at
`scratch/facts/hud_weapon.md:5,25-27` record six exact functions and identify
this leaf as the only new 48-byte exact member.  The Fable HUD C/H files have no
post-commit dirty difference; its only current dirty config changes are
unrelated particle symbols.

The Fable implementation is not directly importable as an owner packet.  It
retains `#define weapon_hud_globals bss_00453ac4` plus a foreign raw-name
`extern`, leaves the public declaration in `hs.c`, and imports larger helper
schemas/bodies that this leaf does not need.

## Authentic type boundary

Current canonical `source/interface/hud_weapon.c:69-78` already carries the
accepted minimal complete allocation shape for the fields known in this wave:

```c
struct weapon_hud_globals
{
	byte reserved[0x1E0];
	long script_flags;
};
```

Its offset and total-size guards prove `script_flags == 0x1E0` and
`sizeof(struct weapon_hud_globals) == 0x1E4`.  This was admitted for the exact
allocator/new-map pair in
`docs/object_matching_logs/hud_weapon_obj_jonas_initialize_interface_20260829.md`.
The setter needs no additional record fields, so the reserved prefix remains
within that already-admitted boundary.

Later Release-PDB/HCEA evidence corroborates the member semantics:

- `src/headers/weapon_hud_globals_definition.h` names a 32-bit
  `script_flags` tail (later two-local-player Xbox layout: offset `0xF0`, total
  `244`; it does not override January's directly proven offset/size);
- `src/headers/hud_crosshair_show_flags.h` DB-verifies
  `_hud_crosshair_show_bit = 0`;
- `src/blam/sound/scripted_hud_show_crosshair.c` performs the same set/clear;
- `crosshair_state` is genuinely a four-byte struct around an anonymous union
  of `int reference_data` and `float frame_index`, and
  `crosshair_hud_state` owns `crosshair_state states[19]` followed by
  `unsigned int render_flags`.

Consequently, Fable's unrelated `long states[19]` flattening is not an
authentic replacement for the crosshair union and must not enter this packet.
No full HUD globals expansion, getters, sound helper, or `hud_fix_weapon_data`
residual is required for the setter.

Relevant HCEA SHA-256 pins:

- `scripted_hud_show_crosshair.c`:
  `a83e3d87d689e9446b022f47177c8f9d30f56e6e9de659ad7d6357623a60a5fb`
- `hud_crosshair_show_flags.h`:
  `1e8fc65892e1882ae9ac1a7f12c2169a8def40fe3a1f2e9e982afaa3031f61c`
- `weapon_hud_globals_definition.h`:
  `1bbeca32ce1540bf3fbbed271f542a72d81b77410d64fe51dcb8a25eb8af44de`
- `crosshair_state.h`:
  `1d544ef7e88da53e121b506d651ea08e3d91b92095fe82b5e448d3400418cb66`
- `crosshair_hud_state.h`:
  `e3a2d1ff16c45bf4b7616a25998782abf3b95f8475a52d9465bd68d21da7f41f`

## API and data ownership

The only canonical declaration is currently the C-local forward declaration at
`source/hs/hs.c:3759-3760`; its evaluator call is at `:6326`.  The proper public
owner is `source/interface/hud_weapon.h`, which currently lacks the prototype,
and `hs.c` currently lacks that include.  Direct current include consumers of
the owner header are:

- `source/interface/hud_weapon.c:56`
- `source/interface/hud.c:91`
- `source/game/players.c:248`

The owner move adds `source/hs/hs.c` as the fourth direct consumer.  The peer
`hud_unit.h` already owns analogous `scripted_hud_show_* (boolean)` APIs, so no
new public wrapper or private facade is warranted.

The semantic pointer name `weapon_hud_globals` is independently supported by
the allocator's January assertion literal and by the later typed PDB record.
Static TU ownership is strongly supported, but not claimed as directly encoded
by the January code bytes: adjacent `source/interface/hud_unit.c:264` owns
`static struct unit_hud_globals *unit_hud_globals = NULL`, and config marks its
immediately preceding datum at `4537024` static.  All known January uses of the
weapon pointer are within `hud_weapon.c`; the public setter is the cross-TU API.
HCEA's generated per-function corpus exposes the pointer with `extern` to link
split reconstructions, so that declaration is not proof of original source
linkage.  This remaining linkage uncertainty is exactly why the semantic
owner-only consumer gate must precede the body.

## Frozen inputs

Canonical inputs read for this intake:

- `source/interface/hud_weapon.c` SHA-256
  `72b1529c3e58b9350ad82f4605b077b275dfa45cd9d4d1fd4c86d62687e854dd`
- `source/interface/hud_weapon.h` SHA-256
  `35d4a972a8130aa7c7ef58178fac4709389bf2e51c42683773501295daf7634b`
- `source/hs/hs.c` SHA-256
  `549d39286bbc4af84873f5fc1972b7ce353cfde5c93863c9f2a3a13aea55adb9`
- canonical base `hud_weapon.obj` SHA-256
  `e7237ffaf439447e05954e726a68bb13e35c2242b07a3cce7a1d87b791c2b9f1`
- prior path-name ledger SHA-256
  `048bcd573c4b632ee9937c51bcf9caba962181aac1fc649f3c4e70792ed4eb3f`
- prior allocator ledger SHA-256
  `22e43f85a0230ed822719d521da0bf3750d33e7f397adcb3243cf9f56c426156`

Fable inputs read:

- `source/interface/hud_weapon.c` SHA-256
  `17fa1fa5d53f6631e24f624eedd49a17bb40a0266da626577a9fe2639c71f85d`
- `scratch/reports/hud_weapon.md` SHA-256
  `817d81e969df61798310cbad51266920683236f05f98c8d470dbfeaeae6597a0`
- `scratch/facts/hud_weapon.md` SHA-256
  `6b7a8d2d931f63f71b88c95d65068be2cff8fe7ccc7cc6f102194c87acc28352`
