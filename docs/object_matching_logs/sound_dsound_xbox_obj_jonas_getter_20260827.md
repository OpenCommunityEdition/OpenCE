# `sound_dsound_xbox.obj` DirectSound getter recovery — 2026-08-27

## Outcome

This bounded shape-transfer wave recovers `_dsound_get` on its first and only
production candidate.  The function is strict-exact at 18 meaningful / 32
padded bytes with two relocations.  `sound_dsound_xbox.obj` remains
`NonMatching`; no object-completion label, semantic exception, data exception,
or parked-function record changes.

The normalized SHA-256 is
`384e7aada8b4cd93cf43c6609bf1a179a34d1843d6afb2fb90a8cd29c5f6c103`.
Both `IMAGE_REL_I386_DIR32` relocations name `_dsound_globals`: offset 1 with
addend zero and offset 7 with addend 30,876 (`0x789C`).

## Evidence and typed reconstruction

The current exact-shape atlas identified January `_dsound_get` as the sole
recipient of an already exact 32-byte/two-relocation body shape.  Target
disassembly and relocation resolution then established the complete contract:

```text
mov al,  byte ptr [dsound_globals]
mov ecx, dword ptr [dsound_globals+0x789C]
neg al
sbb eax, eax
and eax, ecx
ret
```

The function returns the DirectSound pointer when the first-byte initialized
flag is true and null otherwise.  The user-supplied HCEA corpus was also
searched.  It contains the later `platform_sound_dsound` table but no
`dsound_get` implementation or `dsound_globals` layout, so it provides no byte
claim beyond corroborating the subsystem identity.  January remains the
acceptance oracle.

The retained source uses a minimal typed external prefix/suffix layout:

```c
struct dsound_globals_header
{
	boolean initialized;
	byte reserved001[0x789B];
	LPDIRECTSOUND direct_sound;
};
```

An `offsetof` assertion pins `direct_sound` to `0x789C`.  The global is only
declared `extern`, preserving the target relocation spelling and avoiding any
invented BSS ownership.  The public body follows house formatting:

```c
LPDIRECTSOUND dsound_get(
	void)
{
	return dsound_globals.initialized ? dsound_globals.direct_sound : NULL;
}
```

`<xtl.h>` supplies the canonical XDK DirectSound pointer type.  There is no
raw-address access, object/tag access, cast, pointer/integer reconstruction,
pointer or union punning, or layout dereference through an incompatible type.

## Frozen build evidence

The pre-wave candidate object was preserved as
`build/audit/sound_dsound_xbox_getter_baseline_20260827.obj`, SHA-256
`C820E6A50B367A34570EBD559B14A85502C85BCFA751E2FE5EAD65EFAFFB785C`.
The January split target SHA-256 is
`C7B04E9B80B2A397FB3C5562129C2C5B0681F483A659A05B6076D057FDEA7AAC`.

The pinned XDK 3911 compiler passed a no-output `/Zs` syntax check.  Exactly
one ordinary Ninja edge then produced
`build/audit/sound_dsound_xbox_getter_first_shot_20260827.obj`, SHA-256
`5226C88631F91DEFE49BFD2684D3EAA2FCD64B01C179B5D2C1DBDBC41A291155`.
The semantic audit rose by exactly one accepted function, from 4,256 to 4,257.
Hardened comparison proves all padded bytes and both ordered relocation
identities, addresses, types, addends, and ownership exact.

The baseline object had no accepted functions, so this TU has no inherited
exact-code regression surface.  No nonexact caller or private helper is needed
to emit the getter; it is an independent external definition and therefore a
closed retained subset.

No assembly, register/volatile scheduling control, pragma, intrinsic,
attribute, undefined behavior, byte forcing, synthetic anchor, compiler/tool
substitution, object patch, comparator exception, or admission exception is
used.

## Validation and progress

- full Ninja build and progress generation: pass;
- semantic audit: 470 units, 4,343 functions evaluated, 4,197 semantic exact,
  4,257 accepted exact, and zero unit errors;
- campaign progress: 377/833 objects, 4,231/11,060 functions, and
  510,763/2,198,102 meaningful code bytes;
- Halo progress: 275/468 objects, 4,064/7,574 functions, and
  497,849/1,770,166 meaningful code bytes;
- object admission: zero candidates, zero contradictions, zero revocations;
- parked functions: three active, zero stale, zero invalid;
- tooling suite: 179/179 tests pass; and
- `git diff --check`: pass.

No protected Units/vehicles/AI/game-engine source is changed, and nothing is
pushed.
