# `source/sound/sound_classes.obj` strategy ledger

This is a retroactive ledger for work completed before per-object logs became
mandatory. It records only evidence preserved in Git, `config/parked.json`,
source comments, and the current progress report. Per-experiment hashes that
were not retained by the original lane are not invented here.

## Scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/sound/sound_classes.obj`.
- Current ordinary inventory: 11/12 functions exact; 787/980 meaningful code
  bytes credited.
- `_sound_class_get` is the only code residual.
- Ordinary non-code report: `.rdata` 932/932 bytes; `.data` is 2,452 bytes at
  96.32953%. The object is not eligible for `Matching` without a fresh strict
  ownership comparison of that `.data`.

## Parked residual

| Function | Size T/B | Relocs T/B | Hash T/B | First preserved evidence | Class |
|---|---:|---:|---|---|---|
| `_sound_class_get` | `0xD0/0xD0` | `18/18` | `94cb85335dd2d7a8df3858d5f19def3472fc2fe6869825370127078f852d65a0` / `6b1d112cd08fd123c3ce0cdc72232802444600c5e669463144181f64f4572c00` | First relocation target `+0x16`, candidate `+0x13`; remaining code uses an ESI/EDI index-pointer mirror. | register allocation |

## Preserved experiment history

The historical lookup lane reconstructed the definition table, assertions,
and pointer-return behavior. The final source comment and parked record prove
that equal size and all 18 relocation identities were reached, but the lane
did not preserve a row-by-row experiment matrix. Do not claim unrecorded
spelling variants as tested.

Known exhausted class: declaration/index-pointer lifetime spellings that leave
the ESI/EDI mirror unchanged. Unsafe controls remain prohibited: `volatile`
anchors, inline assembly, byte patches, undefined behavior, or flag changes.

## Reopen and disposition

Reopen `_sound_class_get` only with an independently exact donor that explains
the index/pointer register allocation, original local-variable provenance, or
a new defined-C lifetime control. Before any object admission, also resolve
the `.data` ownership/byte discrepancy with the hardened comparator.

Disposition: active residual evidence is parked; the object is **not**
strict-complete and must remain `NonMatching`.
