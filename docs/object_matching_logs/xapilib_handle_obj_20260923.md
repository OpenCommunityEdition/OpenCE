# XAPILIB `handle.obj` — admitted exact source (2026-09-23)

Source: `libs/xapilib/handle.c`. Target: `build/split/libs/xapilib/handle.obj`.

The object consists of two public `BOOL WINAPI` wrappers and three imports.
Public function signatures and types come from the local XDK
`xbox/include/WinBase.h`. The wrappers pass through to the imported Xbox NT
object primitives, return `TRUE` for nonnegative status, and call
`XapiSetLastNTError` before returning `FALSE` otherwise. The target's
`DuplicateHandle` passes its source handle, output pointer, and options to
`NtDuplicateObject`; its other compatibility arguments are unused. This is
visible directly in the target's three push operands. No fabricated global,
manual inline expansion, UB, or compiler-directive steering is involved.

Isolated reproduction (without changing the shared build):

```text
xbox/bin/vc7/CL.Exe /nologo /c /O1 /Gy /D_XBOX /D_X86_ /Ixbox/include /Foscratch/xapi_handle_o1.obj libs/xapilib/handle.c
```

Strict `tools.coff_compare.section_infos_equal` against the split target:

| Function | Target/candidate size | Relocations | Normalized SHA-256 | Exact |
| --- | ---: | --- | --- | --- |
| `_CloseHandle@4` | 30/30 | DIR32 `__imp__NtClose@4` at +6; REL32 `_XapiSetLastNTError@4` at +21 | `bc267c07baa29ce7e5aa6783b7853f9826071805e493faeb72bf3ffedd6abe5e` | yes |
| `_DuplicateHandle@28` | 38/38 | DIR32 `__imp__NtDuplicateObject@12` at +14; REL32 `_XapiSetLastNTError@4` at +29 | `dfa35729983fed68e4b0278a486e605049ef0769fcfee4aa26cd418c8d70b6c8` | yes |

Each target and candidate text section has flags `0x60101020`; there are no
target data sections or private target functions. The two public names,
section-relative offsets, storage classes, and all three import names agree.
The VC7 candidate has only compiler debug/directive sections in addition to
the two target program sections. The full build, owner audit, park validation,
stable zero-regression sweep, and maintained test suite passed; the object is
marked `Matching`.
