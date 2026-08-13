# `libs/libcmt/wcsncpy.obj` exact-match ledger

This atomic lane independently revalidates an older completed result against
the current January target. Historical source is a reconstruction lead, not
admission evidence.

## Plain-English behavior

The CRT routine copies at most `count` UTF-16 characters. If it reaches the
source terminator early, it fills the unused destination elements with zeroes.
It returns the original destination pointer.

## Target inventory

- One public cdecl function, `_wcsncpy`.
- Target padded size: 61 bytes.
- No relocations and no owned `.data`, `.bss`, or `.rdata`.

## Measured source shapes

| Shape | Measurement | Disposition |
|---|---|---|
| Conventional two-phase CRT loop: copy through the terminator, then pad unused elements | Historical lead measured 61/61 bytes, 0/0 relocs | accepted for fresh validation |

The two-phase shape permits VC7 to combine pairs of padding stores into
`rep stosd`, while retaining `rep stosw` for an odd final element. Fresh
current-campaign measurements and all gates are recorded below before
admission.

## Current-campaign strict evidence

The source was independently transplanted onto campaign tip `ce1993b7` and
built with the project-configured XDK 3911 compiler.  The generated command was
exactly:

```text
xbox/bin/vc7/CL.Exe /nologo /c /O1 /Gy /I"xbox/include" \
  /Fobuild\base\libs\libcmt\wcsncpy.obj libs\libcmt\wcsncpy.c
```

No lane-local build flag or build-rule change was made.

| symbol | target | candidate | relocations | normalized SHA-256 | result |
|---|---:|---:|---:|---|---|
| `_wcsncpy` | 61 B | 61 B | 0 / 0 | `35235fb5a6f350890633579106299eb2564e42c7f667b4a02839fbba3e31cb39` | exact |

With no relocations, the function's 61 runtime code bytes are literally
identical, not merely relocation-normalized.  The target has one `.text`
COMDAT with external owner `_wcsncpy`; the candidate has the same runtime
ownership.  The target owns no other section, so there is no unresolved data,
BSS, read-only constant, or anonymous ownership question.  Candidate-only
`.drectve` and debug sections are non-runtime compiler metadata.

## Gates and blast radius

- The complete configured libcmt project built successfully.
- The complete 467-object Halo project built successfully.
- `ninja progress` and `ninja semantic_progress` completed with zero unit
  errors.
- The hardened comparator returned `all_equal: true` for `_wcsncpy`.
- `llvm-readobj` confirmed the one-function/no-data target inventory and exact
  external COMDAT ownership.
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 177 / 177 passed.

This file adds no shared declaration or header, so its source-level blast
radius is one translation unit.  The full builds nevertheless recompiled and
validated the wider configured projects.

## House/Berth and readability audit

The three parameters are each on their own line, and the non-void function has
one explicit final return.  `(void)` no-argument formatting is not applicable.
The implementation is ordinary readable C and contains no assembly,
`volatile`, forced inlining, undefined behavior, byte forcing, alignment
directive, raw structure offset, comparator exception, or per-function flag
workaround.  It has no tag or object datum access requiring typed macros.  No
original January bug was identified, so no bug-preservation comment is needed.
