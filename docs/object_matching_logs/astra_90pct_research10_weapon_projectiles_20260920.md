> Research-only packet10: zero production change and zero exact credit. Latest validated production remains wave9.

# Packet 10 — trigger_create_projectiles: zero-shape closeout

**No new admissible mechanism found. Zero C compilations, zero source shapes, zero gain.** The freshly admitted `_weapon_update` and all other inherited controls remain untouched. The unresolved boundary is still the three natural point-helper operations and their unwanted emitted helper owner, not a missing projectile calculation that fresh evidence newly authenticates.

## Freeze and prior rejection boundary

The frozen current source includes packet9's exact weapon update: SHA-256 `868d3c928ffa1f431491b29e05020fa1f5fc742a4d39c488dc255672c8a87d1b`. The base object has 76 exact target functions, 95 emitted function owners, 127 non-debug named owner records, 33 non-debug noncode sections, empty COMMON, and no `_point_from_line3d`. Source and base object remain byte-identical to that freeze (`audit.json`).

`_trigger_create_projectiles` remains 1,808 padded / 41 relocations, normalized SHA `4d21f1084e455f54d26681a5bb5db433923570ce279cdbdf333cdb5c8815cd5f`; January is 1,822 meaningful / 1,824 padded / 41 relocations, SHA `95877e588ec3a4ece0c2e209024ed795639d60cec25894e5f7dd5fdc7ef7b9a0`. Fresh target/current alignment reports frame `0x1c20` versus `0x1c1c`, 563 versus 568 instructions. Those differences are not a license to insert pressure locals.

Read the complete pertinent histories before extraction: September13 weapons owner park, September14 100K/waveC/150K W1/W3, September19 evidence closeout, and the breakable-surfaces shared point-owner comparison. The archived `w14_weapons` `d_pfl.py`, `t1..t6` and related variants were read directly, alongside the prior wave-C results. They already cover moving `use_aiming_vector` initialization, its block/declaration forms, boolean-expression variants, `inside_bsp` alternatives, manual offset locals and non-emitting helper macro forms. None was repeated.

The known `C:\halo-worktrees\opus5-throughput-recovery-20260916\scratch\workers\w14_weapons\d_pfl.obj` was inspected **without recompiling or adopting its source**. `audit.py` independently confirms the target function is exact and `_point_from_line3d` is emitted, 48 bytes. This is a reference-only historical artifact, not a current candidate: it does not contain packet9's new source. Its old exactness does not satisfy the current no-new-point-owner requirement.

## Fresh actual primary evidence

`collect.py` ran Ghidra 12.1.2 read-only and fresh DIA exports for `trigger_create_projectiles`. The Ghidra function is supplied PE `0x69fc70`; SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. `primary.py` independently reads the original PE bytes and validates all ten RTC records at descriptor `0x6a0698`.

The relevant new capture confirms the **known** operation:

| Supplied raw address | Operation |
| --- | --- |
| `0x6a0051` | point-from-line operation using origin, forward, first-person offset x, origin |
| `0x6a007f` | same operation using origin, left, offset y, origin |
| `0x6a00ad` | same operation using origin, up, offset z, origin |

All three calls target thunk `0x409ab1`, independently resolved to body `0x42e0d0..0x42e12b`. That raw body computes each result component as source component plus vector component times the third float argument, writes through the fourth pointer, and returns it. See `three-origin-operations.raw.asm`, `point-operation-thunk.raw.asm` and `point-operation-body.raw.asm`. The exact 2020 helper symbol name is not recovered from this stripped PE; the operation is unambiguous and corresponds to the existing genuine `point_from_line3d` API. No new helper is proposed.

January `target.asm` `+0x2c9..+0x34c` implements those three successive coordinate offsets inline before `_player_aim_projectile` at `+0x34f`. It has no point-helper symbol in this object. The fresh /Od calls corroborate the prior original-operation diagnosis, but do not explain an authentic context that avoids the extra VC7 COMDAT.

The RTC records name markers (6912 bytes), origin/forward, inherited velocity/error outputs, up/left, first_forward, placement data, and camera_position. Fresh HCEA DIA independently gives `object_marker[64]`, `const char *[2] trigger_marker_names`, the corresponding point/vector/float types, placement data and camera position. This does not expose a missing meaningful scalar output analogous to packet9's quotient/remainder values. Generated Ghidra arrays, artificial scalar names and apparent stack offsets are not source declarations; raw RTC offsets are preserved in `primary.json`.

## Cross-build limits and rejected temptation

HCEA's authenticated signature is `static void trigger_create_projectiles(long, short, NetworkedDatumRole)`, with source `d:\projects\code\hcex\sources\items\weapons.c`, MD5 `9EA3D60FA4F1F61C85A268B760EFCA15`. The later Win32 function also has a third argument, a projectile-definition velocity branch, different object-creation interface, and an extra BSP-call argument. These concrete differences prevent direct transplantation. No January ABI or owner type is changed.

Fresh /Od shows the later placement of `use_aiming_vector = TRUE` and an `inside_bsp` boolean-expression form, but both already have explicit historical probes. Repeating them under the guise of fresh extraction would not solve the owner boundary. The RTC `left` name versus current reconstructed `right` name is not a type/operation difference and provides no justification for changing declaration order or frame packing. No later array/data size is imported merely to alter stack pressure.

## Reopen condition

Reopen only for an independently supported authentic header/helper/call context that preserves the selected `_point_from_line3d` provider and emits no new point owner, or a distinct meaningful operation/lifetime that plausibly closes January without manual helper expansion, fake dependency/pressure locals, forced inlining, or metadata steering. This packet supplies new raw corroboration, not a new compiler experiment. No production change is proposed.
