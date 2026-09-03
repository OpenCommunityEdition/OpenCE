# `bitmap_extract.obj` Fable-backlog reconciliation (2026-09-02)

## Scope

This lane reconstructs the previously unwritten January Xbox bitmap-extraction
translation unit in `source/bitmaps/bitmap_extract.c`.  The pinned baseline had
0 exact, 0 residual, and 21 unwritten target functions.  The retained source is
ordinary typed C; it does not contain register hints, volatile steering,
code-generation pragmas, forced inlining, inline assembly, raw byte emission,
or address-named implemented functions/globals.

The reconstruction was checked against:

- the January target object's instructions, relocations, assertion text, source
  line numbers, and data offsets;
- Aerocatia/Demon's named `bitmap_group` enums and layouts;
- the independently recovered PC bitmap-extraction behavior in Stian's Halo
  history, used as a semantic donor rather than copied as an exact-code claim;
- the campaign's `gate.py`, COFF semantic comparison, and fake-match scanner.

## Retained exact closures

With temporary aliases mapping the new semantic private names to the old split
labels, the final isolated gate reports 6 exact functions totaling 1,744 padded
target bytes:

| Function | Padded bytes |
| --- | ---: |
| `bitmaps_extract_from_plate` | 304 |
| `extract_initialize` | 304 |
| `extract_find_sequence_bounds` | 288 |
| `extract_find_bitmap_bounds` | 400 |
| `extract_without_sequences` | 272 |
| `extract_sequences` | 176 |

`extract_find_bitmap_bounds` becomes live only after the private
`extract_bitmap` body is present.  Defining `extract_bitmap` in its owning
translation unit also restores January's private EAX calling convention in
`extract_without_sequences`; that caller then closes exactly without a forced
calling-convention annotation.

## Honest residuals

- `extract_warn_about_horizontal_border`: 128/128 bytes and 9/9 relocation
  identities.  Four normalized bytes differ because VC7 chose different
  temporary registers for two vararg integer extensions.  Source-shape probes
  did not expose a semantic defect, so the natural typed loop is retained.
- `extract_bitmap`: 1,152 candidate bytes versus 1,168 target bytes, with 66
  relocations on both sides.  The source reconstructs the target-proven bounds,
  registration-point, marker filtering, alpha warnings, DXT1 color-key,
  interface-format validation, pending-entry, sprite, plateless-cube, and
  direct-bitmap paths.  The remaining gap is compiler register/cold-block
  allocation and the still-unwritten TU-private `extract_add_bitmap` callee:
  January passes that callee's bitmap in EAX, whereas an unresolved declaration
  must use an ordinary pushed argument.  The campaign stops here rather than
  inventing register lifetimes or a fake wrapper.
- `bitmaps_extract`: 720 candidate bytes versus 736 target bytes, with 66 versus
  68 relocations.  The retained function has the target's validations,
  allocation/decompression path, sequence dispatch, group-type switch, errors,
  and cleanup.  The residual is cold cleanup/tail layout plus downstream private
  callees, not license to add synthetic control flow.

The target itself confirms the superficially surprising allocation-failure
path: it emits both the bitmap-array allocation error and the subsequent
resize-to-zero error before cleanup.  That behavior is therefore retained and
is not an accidental or invented match.

## Names, types, and ownership

- The 0x2C private BSS owner is named `extract_data`, corroborated by the target
  assertion text `bottom<=extract_data.plate->height`.
- Implemented private routines are semantic and static.  All six renamed but
  still-unwritten helpers are marked private in `config/symbols.json`.  Their
  ordinary declarations in the owning C file are temporary unresolved private
  shims: VC7 cannot accept `static` declarations without definitions, so source
  linkage will become `static` when each body is reconstructed.  Their old
  `code_...` labels are not used in source.
- `bitmap_group` imports use the Demon-corrobated original field spellings
  `format`, `detail_fade`, `import_width`, `import_height`, `import_bitmap`, and
  `smoothing_filter_size` without changing layout.
- `bitmap_group_sprite`, `bitmap_group_sequence`, `bitmap_extract_entry`, and
  `bitmap_extract_data` have compile-time size checks for 0x20, 0x40, 0x10, and
  0x2C respectively.
- The DXT1 alpha carrier is `unsigned long`, matching the target's 32-bit shift
  and comparisons.  General scalar types use CSeries definitions (`boolean`,
  `real`, `word`, `pixel32`, `rectangle2d`) rather than substitute primitives.
- The closest associated header is `source/bitmaps/bitmap_group.h`, but the two
  extraction entry points currently have no outside C caller.  Their initially
  added declarations were removed during canonical admission because VC7's
  declaration-position sensitivity regressed an unrelated exact bitmap utility
  function.  Private prototypes remain in the owner C file.
- `data_decompress` now exposes its target-proven destination-capacity argument.
  Its three-function object remains 3/3 exact.

## Verification before canonical admission

The final isolated bitmap gate reports:

```text
exact 6, residual 3, unwritten 12 (21 target functions)
```

The scoped fake-match scan reports zero review leads.  `git diff --check` is
clean.  The isolated worktree did not carry the ignored Xbox compiler payload,
so the full build was performed during canonical admission.  That sweep also
identified and removed the otherwise-correct but unused public declarations
described above; final strict verdict and test results are recorded in the
canonical admission commit.
