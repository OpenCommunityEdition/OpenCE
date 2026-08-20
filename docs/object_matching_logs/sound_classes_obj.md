# `source/sound/sound_classes.obj` strategy ledger

## Strict closeout

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1 with
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/sound/sound_classes.obj`.
- All 12 functions pass the strict COFF comparator: 1,104 padded code bytes
  and 64 relocations.
- All 38 target-owned non-code sections are accepted: 3,319 logical bytes
  and 51 relocations. Thirty-seven sections are strict fingerprint matches;
  `_sound_classes` has identical normalized bytes and independently resolved
  relocation destinations, with its csplit string-owner attribution recorded
  in `config/semantic_data_matches.json`.
- `_sound_class_data` is now the target's undefined external record rather
  than a candidate-only four-byte COMMON definition.

The final lookup lever is source-faithful and defined C. The local
`definition` pointer remains for both field assertions, while the final return
re-forms the same proven element as `&sound_classes[class_index]`. Keeping the
index live through the return gives VC7 the January ESI-index/EDI-definition
allocation. `_sound_class_get` is exactly 208 padded bytes, 18 relocations,
and normalized SHA-256
`94cb85335dd2d7a8df3858d5f19def3472fc2fe6869825370127078f852d65a0`.

The object is admitted as `Matching`, and its former parked entry is removed.
Future changes must preserve all 12 function fingerprints, all 38 accepted
target-owned data sections, and the complete rebuilt symbol inventory.

This is a retroactive ledger for work completed before per-object logs became
mandatory. It records only evidence preserved in Git, `config/parked.json`,
source comments, and the current progress report. Per-experiment hashes that
were not retained by the original lane are not invented here.

## Historical scope and baseline

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1,
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/sound/sound_classes.obj`.
- Pre-closeout ordinary inventory: 11/12 functions exact; 787/980 meaningful code
  bytes credited.
- `_sound_class_get` was the only code residual.
- Ordinary non-code report: `.rdata` 932/932 bytes; `.data` is 2,452 bytes at
  96.32953%. The object is not eligible for `Matching` without a fresh strict
  ownership comparison of that `.data`.

## Historical parked residual

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

The residual is closed. Reopen only if a later source change fails the strict
whole-unit regression manifest; do not repeat declaration/register sweeps or
weaken the exact return-expression lifetime. The object is strict-complete and
must remain `Matching` while every recorded invariant continues to pass.
