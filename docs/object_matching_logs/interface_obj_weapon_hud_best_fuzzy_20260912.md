# `interface.obj` weapon-HUD resolver best fuzzy (2026-09-12)

## Disposition

`interface_get_weapon_hud_index` is now present as a complete, typed, natural-C
reconstruction.  It is the best evidenced candidate presently available, but
it is **not byte-exact** and receives **zero exact-function and zero exact-byte
credit**.

| function | meaningful | target/candidate padded | target/candidate relocations | result |
| --- | ---: | ---: | ---: | --- |
| `_interface_get_weapon_hud_index` | 444 | 448 / 448 | 20 / 20 | residual, zero credit |

The target normalized SHA-256 is
`0673f867b219cdbd399685310736ae3de8263479cb2104edd5c56f48930c6549`;
the candidate is
`57151d28c46aeee1fc2bb715ebde2fbc994b52447a4ba273d624aa56e7749e72`.
The normalized-byte comparison reports 69 differing bytes, largely branch
displacements after the two-byte instruction-stream drift.  Relocation target
kind and destination sequence are identical.  The target has 144 instructions
and the candidate 145.

The ordinary isolated unit gate is:

```text
residual    448  _interface_get_weapon_hud_index  [sha]
== exact 15  residual 1  unwritten 2  (of 18 listed)
```

All fifteen inherited exact functions remain exact, and the emitted-symbol
guard confirms that `_point_from_line3d` is absent.

## Source and January evidence

The semantic donor is the immutable HCEA reconstruction:

`halocea-review/src/blam/interface/interface_get_weapon_hud_index.c`

Its SHA-256 is
`3895e1c2a1f2a4570d99a7218f7d5762c6badad9efb7e40e10eb194333b8a97f`.
The donor supplies the complete operation and the later-build identity, while
January's `interface.obj` instruction stream and relocations remain the
acceptance authority.  The January symbol atlas and HCEA identify the public
owner and signature as:

```c
long interface_get_weapon_hud_index(
	real *flashlight_power);
```

The declaration now lives in `source/interface/interface.h`; the temporary
same-C-file declaration was removed.  The body uses `player_get`, `unit_get`,
`weapon_get`, `unit_definition_get`, `weapon_definition_get`, and
`TAG_BLOCK_GET_ELEMENT`, not raw datum/tag accesses.  It uses the established
`director_perspective` enum values and `_unit_seat_gunner_bit`, project `real`,
and an explicit returned value.  TU-local semantic HUD views expose
`show_hud` and `defaults.default_weapon_hud`, with a compile-time check that
the tag-reference index is at January offset `0x2CC`; no raw-offset expression
is used by executable code.

The complete behavior is:

1. map the rendered local-player slot to its player datum;
2. reject hidden HUD, neutral/scripted cameras, or a missing player unit;
3. resolve the unit's current inventory weapon;
4. if absent, resolve the parent unit's weapon only from a gunner seat;
5. preserve the player's integrated-light power when its own weapon is used;
6. return the weapon definition's HUD tag, or the global default HUD for an
   otherwise empty inventory; and
7. write the flashlight power on every exit.

## Exact residual boundary

The natural body reproduces every January instruction through the final
out-parameter copies.  January copies the four-byte `real` representation
through general-purpose registers at both duplicated exits:

```text
mov edx,[ebp-4]
mov [eax],edx
```

VC7 lowers the ordinary C assignment `*flashlight_power = flashlight;` through
the x87 stack:

```text
fld  dword ptr [ebp-4]
fstp dword ptr [edx]
```

That also changes the neighboring epilogue schedule and shifts the final two
relocation addresses by two bytes.  The earlier investigation tested `/Op`,
declaration ordering, split initialization, and an explicit early return; none
closed the residual.  Type punning, an aggregate-copy facade, `memcpy`, a
wrong integer-pointer ABI, volatile/register pressure, or inline assembly
would exist only to steer these bytes and is therefore rejected by the house
rules.  Reopen only with original January source or an independently attested
source construct that naturally causes VC7 to use the integer copy.

## Header blast radius

Every direct `interface.h` C consumer was independently compiled with the
isolated campaign gate and the forbidden-point guard:

- `source/interface/interface`
- `source/bink/bink_playback`
- `source/cache/xbox_texture_cache`
- `source/hs/hs_compile`
- `source/hs/hs`
- `source/game/game`
- `source/ai/ai_profile`
- `source/main/main`
- `source/text/draw_string`
- `source/game/game_engine`
- `source/interface/hud_messaging`
- `source/interface/ui_widget`
- `source/interface/hud`
- `source/interface/terminal`
- `source/rasterizer/xbox/rasterizer_xbox_motion_sensor`
- `source/render/render_debug`
- `source/physics/collision_usage`

No baseline exact function was lost in sixteen consumers.  `hs_compile.obj`
currently has three baseline-exact functions reported as relocation-identity
residuals (`hs_parse_tag_reference`, `hs_parse_enum`, and
`hs_parse_object_name`); an A/B gate with the new prototype temporarily
removed produced the same three residuals, proving they are pre-existing
shared-tree state and not caused by this owner declaration.

## Adjacent 400--800-byte leaf review

The remaining interface-family candidates in the requested size band were
reviewed without landing speculative code:

- `hud_weapon.obj::_hud_render_weapon_interface` (496 padded) calls several
  still-unwritten same-TU helpers through January's custom private register
  conventions; reconstructing it alone cannot reproduce the call boundary.
- `hud_draw.obj::_code_000c0e10` (480 padded) is HCEA's
  `hud_draw_multitexture_overlay_get_current_weapon_definition`, but January
  passes its player parameter in EAX through a private convention selected
  with the still-unwritten 2,272-byte caller.
- `hud_draw.obj::_hud_draw_static_element` (640 padded) likewise calls that
  same 2,272-byte private renderer through a custom same-TU convention.
- The 320/192-byte color packers have prior independent rejection records:
  their proposed exact bodies require hand-steered x87 assembly and one
  natural staged form has a proven rounding counterexample.

These are dependency packets, not honest standalone leaves.  No exact credit
or source change is claimed for them here.

## Verification artifacts

- candidate object: `scratch/interface_weapon_hud_fuzzy.obj`, SHA-256
  `48ebb56d1b7179408baa44ac5a0d4311744716356c892f85bd946ffa2f7476ae`
- January split object SHA-256:
  `2d89bac5a4216dc0bdb3c0144acde11606198c14e8e83154eda722795f1b91b6`
- `tools/fake_match_scan.py source/interface/interface.c`: zero review leads
- `git diff --check` on the two source-owner paths: pass

The parked-function manifest must be updated only after the orchestrator's
full build refreshes `build/base` and `build/report.json`; adding the current
scratch measurement before that refresh would create a knowingly stale park.
No configure, Ninja build, commit, or push was performed in this worker.
