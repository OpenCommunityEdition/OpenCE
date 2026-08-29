# `units.obj` — strict final integration and admission evidence

## Result

The integrated `source/units/units.c` reconstruction is complete under the
campaign's hardened COFF comparator:

| Census | Exact | Nonexact | Absent | Bytes | Relocations |
| --- | ---: | ---: | ---: | ---: | ---: |
| January code owners | 189 | 0 | 0 | 54,864 | 2,347 |
| January runtime-data owners | 129 | 0 | 0 | 6,519 | 7 |

`python tools/campaign/gate.py source/units/units` independently reports
`exact 189`, `residual 0`, and `unwritten 0`. The reconstructed January split
object has SHA-256
`de70778800ba600aa7365d9fa789b2d1147baf3b67e50d2f9a70ff958c9fa757`.

No semantic-match entry, data-match entry, parked entry, comparator change, or
tool exception was added for this closeout.

## Final source-policy review

The final integration removes the compiler barrier from
`unit_export_function_values`. Its exact scheduling now comes from ordinary C:
indexed array access plus the two-use `private_loop_frame_index` local. The
source contains no inline assembly, intrinsic/barrier, pragma, `volatile`,
`register`, `__declspec`, raw address, or type-punning workaround.

The final review also replaced all thirteen anonymous flag-bit uses with the
documented engine names. Shared damage and weapon-control enums now live in
their owning headers. Seven previously implicit cross-module calls now have
typed declarations in the appropriate AI, damage, Units, Biped, and Vehicle
headers. These source-only clarity changes preserve the complete 189/129 exact
censuses.

## Runtime ownership details

The writable aggregate owned by `_magic_base_animation_seat_index` compares
strict exact at 1,564 bytes and seven relocations. `_unit_globals` compares
strict exact at four logical BSS bytes and zero relocations.

The split producer necessarily records linked-image owners as external and
materializes zero bytes for its reconstructed BSS section. VC7 naturally emits
the private `unit_globals`, `unit_update_section`, and `last_time` definitions
as storage class 3 and represents BSS with no file-backed raw payload. The
hardened comparison proves identical logical bytes, section flags, relocation
addresses/types/destinations, and owner offsets. This is the established
csplit-versus-compiler representation boundary, not an ownership waiver or a
new exception.

## Legacy objdiff label boundary

The ordinary objdiff report still assigns sub-100 fuzzy percentages to 19
relocation-dense functions and the writable aggregate even though every one is
strict-equal after relocation normalization. Temporarily changing the unit to
`Matching` makes the repository admission audit correctly fail closed and
report that contradiction. Therefore `config/config.json` remains
`NonMatching`; no raw-comparator exception is introduced merely to force a
completion label. The hardened 189/129 census above is the acceptance
authority for this integrated reconstruction.

## Repository validation

The final tree passes:

- forced Units candidate and January split rebuild;
- the 189-function campaign gate;
- complete `halobetacache_build`, `libcmt_build`, and `all_source` builds;
- semantic-progress and ordinary-progress generation;
- object-admission, semantic-match, and parked-function audits;
- the full tools test suite;
- `git diff --check` and the Units source-policy scans.
