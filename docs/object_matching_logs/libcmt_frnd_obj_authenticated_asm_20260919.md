# `libs/libcmt/frnd.obj` authenticated inline-assembly admission

## Result

`libs/libcmt/frnd.c` now contains the readable historical source shape proven
by the XDK 3911 debug records: a `double result` local, a three-instruction x87
inline-assembly block, and an ordinary C `return result;`.  Compiled with the
unit's normal `/O1 /Gy` flags, `__frnd` is strictly identical to January:

- size: `0x13` / `0x13`;
- relocations: `0` / `0`;
- normalized SHA-256:
  `b2acfb60d9432e4ad64781ac801918ada55d9c22650b1504c4ee039ed85da31f`.

This is **authenticated `asm-implemented` provenance**, not ordinary-C
reconstruction credit.  No object bytes were copied or patched: XDK VC7
compiled the source normally.

## Why the earlier hold is lifted

The earlier ledger, `libcmt_frnd_obj.md`, correctly proved that the original
Microsoft CRT source was inline assembly but stopped because the campaign then
prohibited all assembly.  The user-confirmed 2026-09-07 house rule 10 now
admits sparse, evidenced assembly in helper and math functions.  `_frnd` is a
small floating-point math helper, and its primary provenance is unusually
strong:

- XDK 3911 names the source `fpw32\\tran\\i386\\frnd.c`;
- CodeView records name the argument `x` and local `result`;
- consecutive source-line mappings place `fld x`, `frndint`, and
  `fstp result` on lines 36, 37, and 38;
- the archived XDK object and January have identical runtime text and COMDAT
  ownership.

The source therefore transcribes the authenticated historical construct rather
than steering a C compiler toward target bytes.  Fixed-mode `floor`, `ceil`,
integer casts, and modern `rint` spellings would change current-control-word
semantics or exception behavior and remain rejected.

## Source shape

```c
double __cdecl _frnd(
	double x)
{
	double result;

	__asm
	{
		fld x
		frndint
		fstp result
	}

	return result;
}
```

The emitted instruction sequence is:

```text
push ecx
push ecx
fld qword ptr [esp + 0xc]
frndint
fstp qword ptr [esp]
fld qword ptr [esp]
pop ecx
pop ecx
ret
```

## Verification

The candidate was compiled without changing project configuration:

```text
xbox\\bin\\vc7\\CL.Exe /nologo /c /O1 /Gy /I"xbox/include" \
  /Foscratch\\frnd_candidate.obj libs\\libcmt\\frnd.c
```

`tools/coff_compare.py` then reported equal `section_info` records for
`__frnd`: 19 bytes, zero relocations, and the hash above.  A Capstone census
showed the nine instructions listed above in both objects.  The target owns no
runtime data and the candidate emits no additional runtime code or data.
`git diff --check -- libs/libcmt/frnd.c` passes.

## Required coordinated configuration

The integration owner should make the following central changes in the same
verified batch:

1. change config index 639, `libs/libcmt/frnd.c`, from `MISSING` to
   `Matching` using the ordinary libcmt `/O1 /Gy` flags;
2. add a `config/parked.json` provenance entry for unit
   `libs/libcmt/frnd`, function `__frnd`, class `asm-implemented`, with both
   target and base measurements set to the strict values above and
   `objdiff_percent: 100.0`;
3. state explicitly that this classification records admitted original
   assembly and does not count the function as an ordinary-C recovery;
4. run the focused gate, full build, stable regression sweep, park audit,
   fake-source scan, and tool tests after the central edits.

The admission is valid only while the compiler-built function remains strictly
exact and the authenticated three-instruction assembly block is unchanged.
