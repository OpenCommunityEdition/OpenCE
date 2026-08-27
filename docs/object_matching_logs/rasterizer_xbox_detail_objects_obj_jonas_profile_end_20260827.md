# Xbox detail-object profile-end recovery (2026-08-27)

## Result

The first and only ordinary code-producing candidate makes
`__rasterizer_detail_objects_end` strict exact in
`source/rasterizer/xbox/rasterizer_xbox_detail_objects.obj`. The object
advances from 0/20 to 1/20 exact functions and from 0/4,081 to 9/4,081
meaningful code bytes. It remains `NonMatching`; no data, completion-label,
semantic-exception, or parked-function credit is claimed.

## Frozen target and source evidence

January's complete function is a nine-byte cdecl leaf followed by seven NOP
padding bytes:

```text
push 0x15
call _rasterizer_profile_end
pop  ecx
ret
```

The target symbol is external storage class 2/type `0x20`; its lone relocation
is zero-addend `REL32` at function offset `+0x03`. Current
`source/rasterizer/rasterizer.c` independently proves that the public
`rasterizer_detail_objects_end(void)` wrapper calls this Xbox backend owner.
Current `rasterizer_xbox.h` and multiple existing backend users prove the
callee signature `void rasterizer_profile_end(short)`.

The HCEA later-platform sources were searched. They corroborate the public and
private `void(void)` split but implement the backend as an empty platform stub,
so they supply signature/topology provenance only and no January byte claim.
All registered Claude worktrees and current object ledgers were searched; none
contained a prior body or production attempt for this function.

## Retained typed source

The source-local prototype avoids importing unrelated Xbox header state into
this otherwise empty translation unit:

```c
void rasterizer_profile_end(
	short profile);

void _rasterizer_detail_objects_end(
	void)
{
	rasterizer_profile_end(21);
	return;
}
```

The no-argument layout, one-parameter-per-line declaration, and explicit
terminal return follow the current house rules. The body contains no assembly,
`register`, `volatile`, optimizer barrier, pragma, intrinsic, attribute, raw
address, pointer/integer reconstruction, pointer or union pun, undefined
behavior, synthetic anchor, object patch, comparator exception, or byte
forcing.

## Immutable artifacts and strict comparison

- Base commit: `09779a30`.
- Pre-wave source blob: `b868f1d9adb6bf78c0ab76549ffd08f3de33ccff`.
- Frozen retained source blob: `a85a581d14715aed890d8dc2e9972579151a83a1`.
- January split object SHA-256:
  `D1A307F55B0170FF7E25427AEE3A7CA9EB1B9498F821F40C826A4A4A9F023A46`.
- Baseline candidate object SHA-256:
  `18A7F9BFC083688A894A771948BB088ACEE7D42C457BD260A4F9025196178520`.
- First-shot object SHA-256:
  `42D8B829DB179800DCC5EC69711C06B6F2C211A603B45146AD305DC79FFECCF9`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1 with the unchanged natural
  `/O2 /Oy- /DDEBUG /Dxbox` flags and include graph.

The hardened comparator proves all 16 padded bytes, relocation count, address,
type, destination identity, addend, and ownership exact. Target and candidate
share normalized SHA-256
`80465da1be06ad7ef44824e4382cd8b4b5401f72d33e2346c424abf92ed3fbda`.
The exact-set comparison reports this function as the sole `newly_exact`
owner and reports no lost exact function.

## One-shot discipline and validation

`git diff --check` and the production compiler's no-output `/Zs` syntax check
passed before emission. Exactly one normal selected-object compiler edge then
produced the retained artifact. No spelling, prototype, constant, declaration,
flag, or compiler retry occurred.

The complete graph and campaign gates pass:

- 377/833 complete objects overall and 275/468 for Halo;
- 510,790/2,198,102 meaningful code bytes and 4,233/11,060 functions overall;
- 497,876/1,770,166 Halo code bytes and 4,066/7,574 Halo functions;
- data unchanged at 1,836,762/4,176,062 overall;
- semantic audit: 470 units, 4,345 functions evaluated, 4,199 semantic exact,
  4,259 accepted exact, and zero unit errors;
- admission: zero candidates, contradictions, or revocations;
- parks: three active, zero stale, zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

January COFF remains the sole admission authority. No protected path,
configuration entry, completion label, push, amend, or history rewrite is
involved.
