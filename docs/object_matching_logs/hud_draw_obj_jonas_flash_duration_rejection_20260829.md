# `hud_draw.obj` flash-duration first-shot rejection (2026-08-29)

## Result

`_get_flash_duration` was tested once from the clean canonical source using
HCEA's typed `hud_color_definition` donor and then removed immediately. The
semantic expression was:

```c
return (long)(hud_color->flash_period * 30.0f);
```

The local type was size-asserted at `0x20` and its `flash_period` member was
offset-asserted at `+0x08`. The candidate nevertheless emitted a call to
`__ftol2`:

| Measurement | January target | First candidate |
| --- | --- | --- |
| Padded bytes | 32 | 32 |
| Relocations | 1 | 2 |
| Normalized SHA-256 | `dcf81ee7e83bf429645379d2ecd9b0992d7d77fa53b25a791a6c04de16d0e973` | `f7f502b8727022b8959cb4a33f4c43f2bd9fae7f53f8dc44ffcd7f1b59904d99` |

January instead performs the float-to-long conversion inline with x87
`fistp` and has only the `30.0f` relocation. This is a compiler-mode boundary,
not a field-layout or semantic ambiguity. No alternate spelling, intrinsic,
inline assembly, pragma, optimizer barrier, compiler-flag change, or
comparator exception was attempted.

The complete inverse patch was applied. `source/interface/hud_draw.c` is
byte-for-byte back at its canonical Git blob
`f93ba077dd594e10dcae41995979c3a9446aebf9`; the rebuilt unit gate is again
two exact, zero residual, and 21 unwritten functions. Reopen this function
only if lawful translation-unit compiler provenance changes the conversion
mode.

January `hud_draw.obj` SHA-256 is
`27d9adb322c237ea9114cd9a8e4d8b5457de517a7feef20ea6bd7158263b20db`;
the restored canonical rebuilt object SHA-256 is
`2e6143e0798aef85b75eddc8a79352accc3d9faf0660456b5ef207bc86c042b6`.
No HUD source or generated object is retained or staged by this rejection
record.
