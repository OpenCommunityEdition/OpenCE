# XAPILIB `perfctr.obj` — admitted exact source (2026-09-23)

Source: `libs/xapilib/perfctr.c`. Target: `build/split/libs/xapilib/perfctr.obj`.

The object consists of two 21-byte exported `__stdcall` wrappers and two
undefined Xbox kernel imports. Both wrappers store the EDX:EAX 64-bit result
of the corresponding imported kernel query and return `TRUE`. Public types
(`BOOL`, `LARGE_INTEGER`) and signatures follow `xbox/include/WinBase.h`; the
kernel imports are declared here because no available XDK header declares
them, and they are used by no other source file. The source uses the natural
`LONGLONG` return and `QuadPart` store; no byte-steering expression,
manual inlining, private-name guess, or invented global is involved.

Isolated reproduction (without changing the shared build):

```text
xbox/bin/vc7/CL.Exe /nologo /c /O1 /Gy /D_XBOX /D_X86_ /Ixbox/include /Foscratch/perfctr_o1.obj libs/xapilib/perfctr.c
```

Strict `tools.coff_compare.section_infos_equal` against the split target:

| Function | Target/candidate size | Relocations | SHA-256 of normalized code | Exact |
| --- | ---: | --- | --- | --- |
| `_QueryPerformanceCounter@4` | 21/21 | one DIR32 to `__imp__KeQueryPerformanceCounter@0` at +2 | `0fe20a943c529b3e8e3b0220facd31a6747ed020cba72a3fbf2ac626be8d9adc` | yes |
| `_QueryPerformanceFrequency@4` | 21/21 | one DIR32 to `__imp__KeQueryPerformanceFrequency@0` at +2 | `0fe20a943c529b3e8e3b0220facd31a6747ed020cba72a3fbf2ac626be8d9adc` | yes |

Each target and candidate text section has flags `0x60101020`; there are no
target data sections or private target symbols. The two public names, their
section-relative offsets, storage classes, and import names agree. The VC7
candidate also contains compiler-generated debug/directive sections, as do
other accepted compiler-built library objects; these are not target program
sections. The full build, owner audit, park validation, stable zero-regression
sweep, and maintained test suite passed; the object is marked `Matching`.

`bitmapa.obj` was separately investigated but **not** admitted. Its four
bytes are `bsf eax,ecx; ret`, yet a natural naked helper compiled by the
available VC7 compiler pads to 16 bytes and produces a COMDAT that the target
does not have. No source or matching credit was retained for it.
