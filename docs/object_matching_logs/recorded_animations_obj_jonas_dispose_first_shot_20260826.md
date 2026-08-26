# `recorded_animations.obj` Jonas disposal first shot (2026-08-26)

## Frozen pre-production packet

This bounded wave starts from clean cumulative commit
`278d55f9e57f8ccac6d5ced80c22fa29db688f79` on isolated branch
`jonas/recorded-animations-dispose-20260826`. It is intentionally outside the
approval-gated Units/shared-header package and changes no protected path.

The selected owner is `_recorded_animations_dispose`. It is absent from the
current base, is final-link closed, and is the first missing direct dependency
needed by the otherwise source-authenticated `_game_dispose` chain. The frozen
source packet is ordinary typed C:

```c
extern void *animation_threads_debug;

void recorded_animations_dispose(
	void)
{
	if (animation_threads_debug)
	{
		match_free(
			"c:\\halo\\SOURCE\\cutscene\\recorded_animations.c",
			123,
			animation_threads_debug);
		animation_threads_debug = NULL;
	}

	return;
}
```

No storage definition, build file, semantic exception, or parked record is in
scope. Production source scope is limited to
`source/cutscene/recorded_animations.c` and the missing public prototype in
`source/cutscene/recorded_animations.h`. The authenticated target-symbol label
`_animation_threads_debug` is added at decimal file offset `4414636` in
`config/symbols.json`; this names the existing target-owned BSS slot without
changing runtime bytes or creating source-owned storage. This ledger is the
only other tracked path in the wave.

## Baseline and target authority

- Baseline source Git blob:
  `1da9a38950def1486c043e8d15c5a2bb773097fe`.
- January split `recorded_animations.obj` SHA-256:
  `cef157f5d4aa20f06b5e14dc659f44cc75ba29d11274faa4ec74166ee51990f0`.
- Untouched branch-local baseline object SHA-256:
  `b0c18d6ca612bdfa65e3c557957af8cfa1dd1f632a652b0d2fbd0b0cf936f0db`.
- Pre-symbol frozen regression manifest:
  `build/audit/recorded-animations-dispose-first-shot-20260826/pre-manifest.json`,
  SHA-256
  `4468c47a3e1263057e593d9b102910e0cc5791beaccb49a95018c889f3654855`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 for
  80x86, `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

Before the authenticated label is added, csplit represents the pointer slot at
absolute `0x435CAC` as `_debug_recording+4`. January initialization, the
`animation_threads_debug` assertion literal, the HCEA knowledge base, and the
same-build reference all authenticate that slot as the distinct pointer
`animation_threads_debug`. Regenerating only the split target after the label
addition must preserve every byte while spelling both pointer relocations as
`_animation_threads_debug+0`.

The target owner is 36 meaningful / 48 padded bytes with four relocations and
normalized byte SHA-256
`b9b64bb030325c73231a51ab101f24b1d8ce3ec37789c76d2686751f0cc07ed0`:

| Offset | Type | Destination |
| ---: | --- | --- |
| +1 | DIR32 | `_animation_threads_debug` (currently `_debug_recording+4`) |
| +12 | DIR32 | `??_C@_0CO@FNHDDCJP@c?3?2halo?2SOURCE?2cutscene?2recorded@` |
| +18 | REL32 | `_debug_free` |
| +27 | DIR32 | `_animation_threads_debug` (currently `_debug_recording+4`) |

The target literal is exactly
`c:\halo\SOURCE\cutscene\recorded_animations.c`. The target line argument is
`123` (`0x7B`). The existing `match_free` macro expands to the declared `_debug_free`
call without any compiler control. `_debug_free` is already accepted strict
exact, so this owner has complete direct-callee closure. The retained
`_recorded_animations_dispose_from_old_map` is already strict exact at 13
meaningful / 16 padded bytes, two relocations, and normalized SHA-256
`9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc`.

Semantic source corroboration is the HCEA donor at commits `f0b06e55` and
`61165c93`, which authenticates the `animation_threads_debug` name and natural
free/null body. The clean Stian donor at commit
`41c10616b69b982700e0913f21a5137807a03d0e`, local path
`research-cache/stian-halo-cseries-20260820/src/halo/cutscene/recorded_animations.c`.
January PC COFF/disassembly remains byte and relocation authority.

## One-shot and acceptance boundary

The pre-symbol snapshot succeeded before any tracked change. After the
authenticated split label is regenerated and proven byte-preserving, a second
pre-source manifest will freeze the label-normalized target before any
production source change. The frozen source will receive one real VC7 `/Zs`
parse-only check, which emits no object, followed by exactly one normal Ninja production compile of
`build/base/source/cutscene/recorded_animations.obj`. The first generated
object will be preserved immediately under the ignored audit directory before
comparison. There will be no spelling, source-shape, control-flow, or tuning
retry in this wave.

Retention requires all of the following:

1. `_recorded_animations_dispose` has exact padded bytes and complete
   relocation address/type/destination/addend identity;
2. `_recorded_animations_dispose_from_old_map` remains strict exact;
3. the emitted path literal has exact content/owner identity;
4. the frozen regression manifest reports no inherited loss, warning, or
   changed nonexact owner; and
5. source-policy, scope, build, semantic, admission, park, test, and replay
   gates all pass.

Any failure rejects the production source edit wholesale. The first artifact
and this ledger will record the miss, but no second candidate compile is
allowed without a genuinely new evidence-backed wave.

The body uses readable typed C with `void` on its own parameter line, an
explicit terminal return, a direct typed external, and the repository memory
interface. It contains no assembly, `volatile`, `register`, barrier, pragma,
intrinsic, attribute/compiler control, raw address or byte offset,
pointer/integer or union pun, undefined behavior, synthetic anchor, or object
byte forcing.

## Result

Production emission and final validation are pending.
