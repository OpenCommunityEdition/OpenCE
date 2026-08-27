# DirectSound Xbox angle conversion recovery (2026-08-27)

## Result

The first and only ordinary code-producing candidate makes
`_dsound_angle_from_angle` strict exact in
`source/sound/sound_dsound_xbox.obj`. The existing exact `_dsound_get` owner
remains exact, so the object advances from 1/39 to 2/39 exact functions and
from 18/9,571 to 36/9,571 meaningful code bytes. It remains `NonMatching`;
no object-completion, runtime-data, semantic-exception, or parked-function
claim is made.

## Evidence and bounded scope

January defines an external cdecl leaf taking one 32-bit floating argument
and returning a 32-bit integer through `__ftol2`. Its complete meaningful
instruction sequence is:

```text
push ebp
mov  ebp, esp
fld  dword ptr [ebp+8]
fmul dword ptr [__real@42652ee1]
pop  ebp
jmp  __ftol2
```

The literal `0x42652EE1` is exactly `57.295780181884766f`, the single-precision
radians-to-degrees multiplier. The independent Stian reconstruction at commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf` supplies the same natural source
expression and `float`/32-bit-integer signature. HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` was searched as a semantic and type
oracle; its later platform backend does not contain this original-Xbox
DirectSound conversion, so it contributes no source body or byte claim.

The two adjacent attenuation wrappers were deliberately excluded from the
frozen candidate. Both call same-TU helper `_code_001b8c20`, which is not yet
reconstructed in the candidate object; retaining either wrapper alone would
leave the replacement TU without a link-closed owner. The prior documented
`transport_network_available` and `set_random_seed` leaves were also excluded
without compilation because their compiler/topology barriers are already
frozen in their respective ledgers.

The retained body is ordinary typed C:

```c
long dsound_angle_from_angle(
	float angle)
{
	return (long)(angle * 57.29578f);
}
```

It follows the current house style and contains no assembly, `register`,
`volatile`, optimizer barrier, pragma, intrinsic, attribute, raw address,
pointer/integer reconstruction, pointer or union pun, undefined behavior,
synthetic anchor, or byte-forcing construct.

## Immutable artifacts and strict comparison

- Base commit: `ecd0fff2d2aa5374eed7c599678d7c6b7ec0b0d1`.
- Pre-wave source blob: `720cfd7bf6a8bd3a0ececbb04027f73c4ba44043`.
- Frozen retained source blob: `918e3b9b51f896607f53052e434cb99a906c92d8`.
- January split object: 53,937 bytes, SHA-256
  `C7B04E9B80B2A397FB3C5562129C2C5B0681F483A659A05B6076D057FDEA7AAC`.
- Baseline candidate object: 1,566 bytes, SHA-256
  `5226C88631F91DEFE49BFD2684D3EAA2FCD64B01C179B5D2C1DBDBC41A291155`.
- First-shot object: 1,897 bytes, SHA-256
  `973D7FB8B821D07C7813DAAF35BD9F2169B082B7F732BF819BE078EAB340DE6F`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1,
  natural `/O2 /Oy- /DDEBUG /Dxbox` project flags.

The hardened COFF comparator proves exact padded bytes and relocation
identity, address, type, addend, and ownership:

| Function | Meaningful / padded | Relocations | Normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `_dsound_angle_from_angle` | 18 / 32 | 2 | `ea6279c6d854d47a52aaff3dd5306713f73870f27fe26028f56f70f200e2386f` | newly exact |
| `_dsound_get` | 18 / 32 | 2 | `384e7aada8b4cd93cf43c6609bf1a179a34d1843d6afb2fb90a8cd29c5f6c103` | still exact |

The angle relocations are `DIR32` at `+0x08` to `__real@42652ee1` and
`REL32` at `+0x0E` to `__ftol2`, both with zero addends. The candidate's
canonical select-any constant owner normalizes to the same symbolic literal.
The exact-set comparison reports one `newly_exact`, one `still_exact`, and no
lost exact function.

## One-shot discipline and validation

Before emission, `git diff --check` and the production compiler `/Zs` syntax
pass succeeded. The selected-object dry boundary was one normal compiler edge.
That edge was invoked once after the source packet was frozen; there was no
source-shape, declaration, flag, or compiler retry.

The complete Halo and library graph passes. Canonical progress becomes:

- 377/833 complete objects overall and 275/468 for Halo;
- 510,781/2,198,102 meaningful code bytes and 4,232/11,060 functions overall;
- 497,867/1,770,166 meaningful Halo code bytes and 4,065/7,574 Halo functions;
- 1,836,762/4,176,062 data bytes overall, unchanged by this wave;
- semantic audit: 470 units, 4,344 functions evaluated, 4,198 semantic exact,
  4,258 accepted exact, and zero unit errors;
- admission: zero candidates, zero contradictions, and zero revocations;
- parks: three active, zero stale, and zero invalid;
- tooling tests: 179/179 pass, with only the inherited pytest cache-permission
  warning.

The applicable Claude campaign documents remain the binary-first, natural-C,
prototype-inference, and fail-closed comparison authority:
`C:/halo-worktrees/claude-ai-conversation-2/AGENTS.md`, `CLAUDE.md`,
`docs/verification_policy.md`, `docs/references/prototype-inference.md`, and
`docs/references/kb-update-policy.md`. January bytes and relocations remain the
sole admission authority. No push, amend, history rewrite, or protected-source
edit is performed.
