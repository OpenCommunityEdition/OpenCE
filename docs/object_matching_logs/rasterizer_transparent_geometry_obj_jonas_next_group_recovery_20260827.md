# `rasterizer_transparent_geometry.obj` next-group recovery

## Result

This checkpoint restores the readable, typed
`rasterizer_transparent_geometry_next_group` body while keeping the object
honestly partial. The January function is not strict exact, so it receives no
matching credit and neither `config/config.json` nor a semantic exception is
changed.

The retained body is 157 meaningful / 160 padded bytes with 12 relocations,
the same size and complete ordered relocation identity/address/type/addend
sequence as January. Its normalized SHA-256 is
`6c2cd1e5846f69b55aa1a2182cf9f0ba47842459b39788263f6ed8fa31c30c47`;
January's is
`bb7cb8a6e864760c768872d911609c2ffd98a894e67b681005f3a0c68af9fb7b`.
Four normalized byte positions differ, all caused by the null-return epilogue:
January schedules `xor eax,eax` before `pop edi`, while the compiler schedules
the independent instructions in the opposite order for the retained source.

The object remains 16/19 strict functions. All 16 inherited exact functions
remain exact after the new definition. The other residuals remain
`rasterizer_transparent_geometry_initialize` (absent) and `_code_00174120`
(176 padded bytes / 14 relocations, nonexact).

## Evidence and source decision

The complete prior object record was reviewed before compiling:

- `rasterizer_transparent_geometry_codex_checkpoint.md` records the historical
  result-variable, flat-return, nested-return, and pre-cast experiments;
- `rasterizer_transparent_geometry_hcea_checkpoint.md` supplies the typed group
  identities and later-build semantic topology;
- `rasterizer_transparent_geometry_obj_jonas_sort_driver_lifetime_20260825.md`
  and `rasterizer_transparent_geometry_obj_jonas_draw_20260826.md` define the
  current strict baseline and exhausted source families;
- Claude's object-specific research at
  `claude/near-complete-objects-20260816@74a085e9`, including
  `research/transparent_geometry_recovery.md` and
  `research/transparent_geometry_sort_comparator.md`, documents the old
  seven-byte near match and its tested variants;
- the current Stian lift at
  `build/audit/refs/stian-halo@fc11d861:src/halo/rasterizer/rasterizer_text.c`
  provides a genuinely new local-lifetime clue: a separate 16-bit
  `sorted_index` local precedes `next_index`, while the assertion independently
  reloads the full 32-bit group field.

January's disassembly confirms that clue. It loads the low word at group
offset `0x90` into `SI`, reloads the dword at the same offset into `EAX`, then
increments `SI`. The old single casted expression could not express those two
simultaneous values. The retained C therefore uses two ordinary `short`
locals, typed `group->sorted_index` access, and the existing typed group/global
arrays. It contains no raw offset access, pointer/integer reconstruction,
representation pun, assembly, `register`, `volatile`, pragma change, barrier,
intrinsic, annotation, byte forcing, or synthetic reference.

Exactly one normal code-producing compile was performed for this frozen
evidence wave. It improved the historical seven-byte residual to four bytes
and reproduced January instruction-for-instruction through the return paths,
apart from the final independent epilogue ordering. No source-spelling retry
was attempted. Reopen that last tie only with new January-authentic source or
compiler evidence that explains the epilogue schedule.

## Reproducibility and gates

- Target object raw SHA-256:
  `d1bcf0d85a17c27f0cb9797931187fad486c2ac7b8ce7bcb43e706422c160722`.
- Retained rebuilt object raw SHA-256:
  `5b82150081ebee4d9b6271f72c70665c3ade2f3b705e5362a725a601af4385c8`.
- Frozen pre-shot baseline object:
  `build/audit/rasterizer_transparent_next_group_baseline_20260827.obj`, raw
  SHA-256
  `a98e6c7a59c8c3fcbd0a58ef88fedba314e337adfa23c4249d38bdb8f8dfb973`.
- XDK 3911 `/Zs` parse gate: pass without warnings; the frozen object was
  unchanged before the one production compile.
- Strict semantic-COFF replay: all 16 inherited exact functions remain exact;
  the retained routine has all 12 exact relocation identities and addresses.
- Halo and libcmt build graphs: pass.
- Semantic audit: 470 units, 4,400 functions evaluated, 4,254 semantic exact,
  4,314 accepted exact, and zero unit errors.
- Campaign progress remains 377/833 objects, 4,287/11,060 functions, and
  515,333/2,198,102 code bytes. Halo remains 275/468 objects,
  4,120/7,574 functions, and 502,419/1,770,166 code bytes.
- Admission audit: zero candidates, contradictions, and revocations.
- Parked audit: three active, zero stale, and zero invalid entries.
- Tooling tests: 179/179 pass, with only the inherited unwritable pytest-cache
  warning.
- `git diff --check`: pass.

This checkpoint claims one additional evaluated, semantically recovered source
body and no exact function, data, or object completion.
