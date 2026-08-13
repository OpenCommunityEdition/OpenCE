# `libs/libcmt/wcsncmp.obj` exact-match ledger

Historical source was used only as a lead; this lane independently rebuilds
and validates the complete object against the January target.

## Plain-English behavior and target inventory

`wcsncmp` compares at most `count` UTF-16 characters. A zero count means the
two spans compare equal. Otherwise it advances while characters are equal and
nonzero, then returns the unsigned character difference.

- One public function, `_wcsncmp`: 53 target bytes, zero relocations.
- No target-owned `.data`, `.bss`, or `.rdata`.

## Measured source shapes

| Shape | Measurement | Disposition |
|---|---|---|
| Result local and one source return | 53/53 bytes and 0/0 relocs, but different branch topology/register assignment | rejected |
| Natural CRT early return for `count == 0`, then loop and difference return | Historical lead strict exact; freshly validated below | accepted |

The target has two physical returns. This is a narrow exception to the
single-return preference: forcing one source return changes codegen and is less
natural for this standard guard case.

## Fresh current-campaign evidence

The source was transplanted onto campaign tip `b935e2dc` and rebuilt with the
project-configured XDK 3911 compiler.  The generated command was:

```text
xbox/bin/vc7/CL.Exe /nologo /c /O1 /Gy /I"xbox/include" \
  /Fobuild\base\libs\libcmt\wcsncmp.obj libs\libcmt\wcsncmp.c
```

No lane-local option or build-rule change was used.

| symbol | target | candidate | relocations | normalized SHA-256 | result |
|---|---:|---:|---:|---|---|
| `_wcsncmp` | 53 B | 53 B | 0 / 0 | `ededa41b5aab23d5f0e6d5418bde779046e5ea1aeab8ed69b2a9f91793113b9a` | exact |

With no relocations, all 53 runtime code bytes are literally identical.
`llvm-readobj` confirms one external `_wcsncmp` `.text` COMDAT on both sides.
The target owns no runtime data or other section; candidate-only directive and
debug sections are compiler metadata.

## Gates and blast radius

- Complete libcmt build: passed.
- Complete 467-object Halo build: passed.
- Progress and semantic progress: passed with zero unit errors.
- Hardened comparator: `all_equal: true`.
- Tool tests: 177 / 177 passed.

The implementation adds no shared header or declaration, so its source-level
blast radius is this one translation unit. Full builds still validate the wider
projects.

## House/Berth and readability audit

All three parameters are on separate lines. The two returns are the documented
and measured exception to the single-return preference; a one-return spelling
changes target codegen. `(void)` formatting is not applicable. The source uses
no assembly, `volatile`, forced inlining, undefined behavior, byte forcing,
alignment directive, raw structure offset, comparator exception, or local flag
workaround. There is no tag/object access requiring typed macros and no
identified January behavioral bug requiring a preservation note.
