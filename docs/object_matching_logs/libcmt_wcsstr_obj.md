# `libs/libcmt/wcsstr.obj` exact-match ledger

Historical source is treated only as a reconstruction lead. This atomic lane
independently rebuilds and validates the complete object against January.

## Plain-English behavior and target inventory

`wcsstr` searches a UTF-16 string for the first occurrence of another UTF-16
string. An empty substring matches at the beginning, and failure returns null.

- One public cdecl function, `_wcsstr`: 94 target bytes, zero relocations.
- No target-owned `.data`, `.bss`, or `.rdata`.

## Measured source shapes

| Shape | Measurement | Disposition |
|---|---|---|
| Inner character test spelled `*left == *right` | Historical measurement 87/94 bytes; VC7 compares directly against memory | rejected |
| Historical CRT spelling `!(*left - *right)` | Materializes both unsigned 16-bit values and subtracts them; historical strict result 94/94 | accepted for fresh validation |

The empty-needle, match, and failure exits mirror the target control flow and
are a measured exception to the preferred single-return style.

## Fresh current-campaign evidence

The source was transplanted onto campaign tip `114ad5d0` and rebuilt with the
project-configured XDK 3911 compiler:

```text
xbox/bin/vc7/CL.Exe /nologo /c /O1 /Gy /I"xbox/include" \
  /Fobuild\base\libs\libcmt\wcsstr.obj libs\libcmt\wcsstr.c
```

No lane-local option or build-rule change was made.

| symbol | target | candidate | relocations | normalized SHA-256 | result |
|---|---:|---:|---:|---|---|
| `_wcsstr` | 94 B | 94 B | 0 / 0 | `1375f1d9e4548ae56a29ece864f13a6fd2d2aafd5945e1fcbdc719e22fe0f6cc` | exact |

Since there are no relocations, all 94 runtime bytes are literally identical.
`llvm-readobj` confirms one external `_wcsstr` `.text` COMDAT on each side.
The target owns no runtime data; candidate-only directive/debug sections are
compiler metadata.

## Gates and blast radius

- Complete libcmt build: passed.
- Complete 467-object Halo build: passed.
- Progress and semantic progress: passed with zero unit errors.
- Hardened comparator: `all_equal: true`.
- Tool tests: 177 / 177 passed.

No shared header or declaration was changed, so the source-level blast radius
is this one translation unit. Full builds still validate both projects.

## House/Berth and readability audit

Both parameters are on their own lines. The natural empty-substring, found,
and failure exits are the documented measured exception to the single-return
preference; combining them changes target topology. `(void)` formatting is not
applicable. The implementation contains no assembly, `volatile`, forced
inlining, undefined behavior, byte forcing, alignment directive, raw structure
offset, comparator exception, or local flag workaround. No tag/object access or
original January behavioral bug occurs here.
