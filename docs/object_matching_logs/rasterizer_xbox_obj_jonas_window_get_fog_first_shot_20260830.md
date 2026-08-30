# `rasterizer_xbox.obj` window-fog first-shot boundary (2026-08-30)

## Scope and evidence

This strict lane froze exactly
`source/rasterizer/xbox/rasterizer_xbox.obj::__rasterizer_window_get_fog`.
Vehicles, Units, Matrix Math, AI Debug, concurrent agent reservations, and all
previously frozen functions remained excluded.  The January split object is
71,925 bytes with SHA-256
`039e8296db80d1391e59fefc49618513926edbbe4e9621d8b7202182`.

January proves a public cdecl destination pointer, a null assertion at source
line `0x5A0`, and a 20-dword structure copy from
`_global_window_parameters+0x1E8`.  The complete function is 60 meaningful / 64
padded bytes with five ordered relocations: the source-path literal, `fog`
assertion literal, `_display_assert`, `_system_exit`, and
`_global_window_parameters+0x1E8`.  The existing typed definitions independently
prove that `struct render_fog` is `0x50` bytes and that the `fog` member of
`struct rasterizer_window_begin_parameters` is at `0x1E8`.  HCEA authenticates
the one-parameter public/private wrapper topology but contains no authoritative
January private body.

The complete applicable Claude campaign, ABI, prototype-inference, compiler,
and verification documentation and all three prior ledgers for this object were
reviewed before mutation.  Repository-wide source, ledger, and history searches
found no prior body or production compile for this function.

## Sole candidate and compiler boundary

One readable typed C89 candidate was frozen.  It used compile-time size and
offset proofs, a typed external `global_window_parameters`, the January
`match_assert` spelling, and an ordinary structure assignment:

```c
void _rasterizer_window_get_fog(
	struct render_fog *fog)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox.c",
		0x5A0,
		fog);
	*fog = global_window_parameters.fog;
	return;
}
```

It contained no assembly, volatile/register/pragma/intrinsic/attribute,
compiler control, raw address or raw offset access, pointer reconstruction,
pun, undefined behavior, byte forcing, or comparator exception.

The sole production VC7 compile did not emit an object.  Including the current
typed campaign headers after `<xtl.h>` exposed an XDK/header-order collision:

```text
source\cseries\cseries.h(44) : error C2059: syntax error : 'constant'
source\math\real_math.h(748) : error C2065: 'M_PI' : undeclared identifier
```

The first error is at cseries' `LONG_MAX` enumerator after the XDK headers have
already established conflicting definitions; the second is consistent with
`_USE_MATH_DEFINES` arriving after the XDK include chain has already consumed
the math header guard.  This is a typed-header/include-order boundary before
code generation, so no byte or relocation comparison was possible.  There was
no include-order, private-prefix, declaration, spelling, source-shape, flag, or
compiler retry.

Reopen only with authenticated same-compiler source evidence fixing the
translation-unit header topology before a production compile.  Do not tune the
body or retry by replacing the typed owners with raw byte layouts.

## Restoration

The candidate, includes, compile-time proofs, and external declaration were
removed by inverse patch.  The restored source is exactly Git blob
`abfec76012f3d29bde4e85d35e10c64c819edb63`; its scoped Git diff is empty.
The preserved baseline candidate object was restored at 2,596 bytes with
SHA-256
`588c4211460d24dc0fd6328ac59fe9ff60eeb1e18af1b0f090608b6c46cadb7a`.
Only this additive boundary ledger is
retained.
