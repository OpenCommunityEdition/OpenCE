# `hud_weapon.obj` Jonas path-name leaf

## Result

The ordinary `strip_path_name` implementation is strict exact. It advances
`source/interface/hud_weapon.obj` from 2/16 to 3/16 exact functions; the
object remains incomplete.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_strip_path_name` | 31 / 32 | 1 | `a913bed76f871042c85f7d40e55b991517729aac4a323e2f5594929f16eedff1` |

The sole zero-addend `REL32` relocation is at `+0x0B` and targets `_strrchr`.
Direct hardened COFF comparison reports `all_equal: true` for all 32 padded
bytes, the relocation manifest, and the normalized hash.

## Source shape

The retained typed C finds the last backslash with `strrchr`, returns the
following character when a separator exists, and otherwise returns the
original path. It uses the standard string interface, has its parameter on
its own line, and ends with an explicit return.

An adjacent first candidate for `_scripted_hud_show_crosshair` was rejected
and removed. Although semantically correct and equal in padded size and
relocation count, VC7 selected the opposite EAX/ECX allocation and January's
relocation names an anonymous address-derived BSS owner. Recreating that name
would violate the campaign's no-raw-address rule. The retained replay has no
residual function and emits no new runtime data owner.

The accepted implementation introduces no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, forced compiler option, raw address or offset,
type punning, synthetic caller, byte forcing, object patch, comparator
exception, or tool exception. `matrix_math.obj`, Vehicles, Units, and the
protected AI and game-engine sources were not edited.
