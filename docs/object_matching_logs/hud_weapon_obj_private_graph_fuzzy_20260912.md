# `hud_weapon.obj` private HUD graph reconstruction (2026-09-12)

## Result

`source/interface/hud_weapon.c` now contains coherent implementations of the four previously missing private owners:

- `render_grenade_hud` (`0x2E0`, 736 padded target bytes)
- `crosshairs_draw` (`0x8D0`, 2256 padded target bytes)
- `hud_update_weapon_local_player` (`0x5C0`, 1472 padded target bytes)
- `render_weapon_hud` (`0xA60`, 2656 padded target bytes)

The implementations use the January target object as the ABI and instruction-order authority and the corresponding HCEA reconstruction only as semantic evidence. HCEA-only globals and behavior (`hcex_off_hud_element`, `hcex_coop_local_player_index`, and later-build static-element fixups) were deliberately omitted. Typed weapon, unit, tag-block, bitmap, and scenario access is retained.

This packet is **best coherent fuzzy source and receives zero exact-byte credit**. It does not claim that size proximity is equivalence.

## Independent gate

Production source compiles successfully and preserves all ten inherited exact functions:

```text
python tools/campaign/gate.py source/interface/hud_weapon --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/hud-weapon-agent-final.obj

exact 10, residual 2, target-label-unwritten 4
forbidden point_from_line3d emission: passed
```

The four entries appear unwritten in that run only because the frozen January split still carries `_code_*` labels. An in-memory name-alias comparison reports:

| semantic owner | frozen target owner | target padded | candidate padded | target/candidate relocations | disposition |
|---|---|---:|---:|---:|---|
| `render_grenade_hud` | `_code_000c8070` | 736 | 736 | equal | fuzzy, zero credit |
| `crosshairs_draw` | `_code_000c8370` | 2256 | 2192 | 76 / 78 | fuzzy, zero credit |
| `hud_update_weapon_local_player` | `_code_000c8ce0` | 1472 | 1456 | 68 / 69 | fuzzy, zero credit |
| `render_weapon_hud` | `_code_000c92a0` | 2656 | 2624 | 84 / 85 | fuzzy, zero credit |

The public callers remain honest residuals:

- `hud_update_weapon`: target 624, candidate 608, target/candidate relocations 34/35.
- `hud_render_weapon_interface`: target 496, candidate 480, target/candidate relocations 24/25.

## Authenticated private names

The four semantic names above are authenticated by the HCEA/PDB source map and their unique caller graph and have been recorded as private (`"static": true`) owners in `config/symbols.json`. No `code + address` names were introduced in source.

## Exact-closure blocker to test separately

`render_grenade_hud` has identical padded size, relocation count, and 242-instruction topology. Its first material instruction-selection divergence is the call result test for `weapon_prevents_grenade_throwing`: the January target uses the byte-sized Boolean ABI (`test al, al`), while this translation unit currently sees no declaration from the proper owner header and therefore compiles the call as implicit `int` (`test eax, eax`).

The correct fix belongs in `source/items/weapons.h`:

```c
boolean weapon_prevents_grenade_throwing(
	long weapon_index);
```

That shared-header change was not retained here. It must be tested as an isolated all-includer/full-sweep packet; a temporary proof compile corrected the return-width divergence but did not by itself close the remaining register-allocation residual.

## Remaining research

- Compare the four semantic owners after regenerating the split names, so recursion and private-call relocation identity are measured directly.
- For `render_grenade_hud`, tune only source schedules that preserve the reconstructed logic after the proper Boolean prototype is safely integrated.
- For the three large owners, use alignment-aware instruction diffs rather than the index-zipped gate display. The current bodies are retained because they restore the complete production caller graph and remain close in size and relocation shape, not because they are exact.
