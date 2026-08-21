# Small leaf harvest Codex checkpoint

This checkpoint records seven independently strict-exact leaves recovered on
2026-08-20 from the preserved `claude/finish-hs-20260816` source state and
revalidated against campaign base `c335fd8e`. Each owning object remains
`NonMatching`; this is partial-function evidence only.

## Strict results

`tools/coff_compare.py` reports exact padded size, normalized bytes,
relocation count, and relocation identity for every retained function.

| Object | Function | Meaningful bytes | Padded bytes | Relocations |
| --- | --- | ---: | ---: | ---: |
| `weather_particle_systems.obj` | `_weather_particle_systems_initialize_for_new_map` | 101 | 112 | 8 |
| `hud_draw.obj` | `_check_stack_buffer` | 40 | 48 | 0 |
| `items.obj` | `_code_000e6150` | 1 | 16 | 0 |
| `main.obj` | `_main_reset_map_private` | 64 | 64 | 10 |
| `models.obj` | `_code_00113710` | 1 | 16 | 0 |
| `rasterizer_text.obj` | `_code_00172e80` | 1 | 16 | 0 |
| `rasterizer_text.obj` | `_code_00172e90` | 1 | 16 | 0 |
| **Total** | **7 functions** | **209** | **288** | **18** |

The weather edit gives the typed private runtime object its target-backed
owner name, `_bss_0043d590`. The complete 628-byte BSS section is independently
strict-exact with zero relocations. Ordinary progress already credited those
628 zero-filled bytes before this ownership correction, so the campaign data
total does not increase.

The weather and main functions were also already accepted through the reviewed
ordinary-exact ledger: their normalized bytes and relocation shapes were
correct, but the private data owner and callee spellings respectively differed.
Making those spellings target-backed converts both to strict exact without
double-counting them. The net progress gain is therefore five functions and 44
meaningful code bytes, while the strict evidence gain is all seven functions.

## Source and policy audit

- The weather state remains a typed private structure, with no raw-address
  access or invented cast.
- `main_reset_map_private` calls the target-backed external
  `_code_000ef8e0` symbol through an ordinary typed prototype.
- `check_stack_buffer` is a bounded typed reverse scan over 128 `long` values.
- The four one-byte leaves are ordinary explicit-return no-op definitions.
- No inline assembly, volatile scheduling device, force-inline annotation,
  optimizer pragma/barrier, type-punning, alignment trick, undefined behavior,
  or byte-forcing construct is present.
- No configuration, semantic exception, parked-function record, frozen large
  object, or pre-existing Markdown file is changed.

## Validation

- Full Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 3,887 functions evaluated, 3,748 semantic exact,
  3,810 accepted exact, and zero unit errors.
- Progress: 375/833 complete objects, 3,799/11,060 exact functions,
  460,961/2,198,102 exact code bytes, and 1,832,148/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, and zero invalid entries.
- Tool suite: 179/179 tests pass.
- A clean post-commit forced-object regression snapshot/check is required and
  recorded in the final handoff.

No GitHub push is performed.
