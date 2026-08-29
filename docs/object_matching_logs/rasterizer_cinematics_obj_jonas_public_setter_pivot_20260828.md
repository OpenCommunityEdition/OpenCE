# `rasterizer_cinematics.obj` public-setter pivot (2026-08-28)

## Scope and result

This bounded lane began at integration commit
`770f1c711344a4c0596678a9cddcc4c0a19c3f7e` in isolated worktree
`work/render-particles-closeout2-20260828`, branch
`jonas/render-particles-closeout2-20260828`. It first closed the requested
`render_particles.obj` evidence audit without compiling its documented fixed
point, then pivoted to one frozen four-function public-donor batch in
`source/rasterizer/rasterizer_cinematics.c`.

Exactly one first-shot batch was emitted. One body was strict exact and is
retained; the three residual bodies and their unused support declarations were
removed immediately without tuning or a second source candidate.

| Function | Target padded bytes | Target relocations | Target normalized SHA-256 | First-shot result |
| --- | ---: | ---: | --- | --- |
| `_rasterizer_screen_effect_set_convolution` | 112 | 4 | `7beef58f5328b7768b506801a5528055389723414ad6c8ea1e4eead421e8aa48` | rejected: 96 bytes, 3 relocations, nonexact normalized bytes |
| `_rasterizer_screen_effect_set_filter` | 112 | 4 | `de4709a02eba43f11818fd1edf0124d200de56a81157be6670dfa8fff0834794` | rejected: 112 bytes, 3 relocations, nonexact normalized bytes |
| `_rasterizer_screen_effect_set_filter_desaturation_tint` | 32 | 1 | `bf144795e5e8ef9153e379304e70c772124a4157f6275c951212b3e099d60c5f` | **retained: strict exact** |
| `_rasterizer_screen_effect_set_video` | 288 | 20 | `eb1b757f0f5e5dbfb23a66ea08ec4cd3b8eeb4ce6385ea9c8a0f3e4e5916c46a` | rejected: 272 bytes, 16 relocations, nonexact normalized bytes |

The retained tint setter is 32 meaningful / 32 padded bytes with one `DIR32`
relocation at `+0x04` to target-owned `_bss_004662f4+0`. Its rebuilt
fingerprint is identical to the target fingerprint above. The object advances
from 6/16 to 7/16 strict-exact functions, a gain of one function and 32 code
bytes. No object-completion, storage, semantic-exception, or parked credit is
claimed.

## `render_particles.obj` fixed-point adjudication

No `_render_particles` candidate was compiled. The complete historical ledger
at `16f8233923d42203e689d6f131b9d80f1ad5314a` and all four prior worktrees were
read before adjudication:

- `render-particles-atomic-20260812` at `16f8233923d42203e689d6f131b9d80f1ad5314a`;
- `render-particles-flags-lab-20260812` at `e2dafe97f32236b8b5d92f01d039745b4d0ce60d`;
- `render-particles-slots2-20260812` at the same base, including its uncommitted
  slot-colouring ledger; and
- `render-particles-salvage-20260820` at
  `468302fb4006beaa4a575f963baa4e200ca7f63a`.

The January target disassembly was read through the complete 1,168-byte body.
The historical best already has the exact `0x24E8` frame, 1,168 padded bytes,
39 relocation addresses and identities, pointer-induction topology, call
sequence, and flag dataflow. Its remaining 45 positional differences are
exclusively compiler-selected stack-slot displacements. Prior lanes exhausted
the evidence-backed declaration, lexical-scope, flag, and pointer-lifetime
controls, including the HCEA PDB fact that one `current_sprite_group` pointer
was reused across both passes.

Authenticated HCEA commit `c168af2e747d3095d9a29418ae401f3a39544863`
was checked directly. Its `render_particles.c` SHA-256 is
`103eb8d0bbbc0ba0ae13a62b8d2276836751cb645ed7033053a1d2782857f21a`.
It remains a later disassembly reconstruction with independent scratch arrays,
forward-index grouping, early returns, a zero-count division guard, and
different flag construction. Those forms are contradicted by January or were
already measured. Authenticated Stian commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf` and Pastudan commit
`918af885935ec470a31256ecce9a977b12b01f80` contain no renderer implementation;
their particle sources add no January local-variable provenance.

The fixed point therefore remains 2/3 exact functions plus the exact
1,528-byte profile owner. Reopen only for January-era source, January local
variable records, or a distinct proven lifetime topology that retains the
required pointer induction.

## Cinematics evidence and candidate boundary

The four screened HCEA donor files at commit `c168af2` have these SHA-256
fingerprints:

- convolution: `897643c3c2f21e81660d91ddacfdb25a1e7cc95e79d0db8a927efe11862bb0f4`;
- filter: `0ffff0d6e66cfcf267d8a4bba78fb6e74ef10ad296676a1edf0580f5047e1e35`;
- desaturation tint: `48b2654a6ed4fdb7d3eba67a4fa483415a0a485520e67407ad6a042260b48c46`;
- video: `82b0aa852e6ff078e820f609c2aacbd001d90da0afdd61c1793564232bf4f152`.

January COFF and disassembly remained authoritative for parameter widths,
field offsets, stores, strings, calls, padding, and relocation identities. The
retained setter uses a typed 0x38 parameter view with the RGB tint at `+0x14`,
inside the already authenticated 0x78 state. Compile-time size and offset
checks cover that access. The public ABI matches the exact HaloScript caller:
three `real` arguments in ordinary cdecl stack slots.

The three rejected natural donor bodies are absent. Their first-shot misses
show that later HCEA semantics do not reproduce January's complete store/call
schedule without new evidence; no spelling, lifetime, declaration-order, or
code-generation experiment was attempted.

## Policy and reproducibility

The retained source uses `void` explicitly, one parameter per line, an explicit
terminal `return;`, a named typed field, and no raw address or object-byte
access. It contains no assembly, `volatile`, `register`, pragma, intrinsic,
barrier, force-inline directive, union/pointer pun, undefined behavior,
synthetic anchor, byte forcing, or private ABI.

- January split-object SHA-256:
  `257b38976236eb2fb5913f4bda88186c6b4de1d498c5677eddded99b5b7e6400`.
- Final checkpoint-rebuilt object SHA-256:
  `3219d6b793286562e9772b69fd66d5d0f15f80f76b799c4282359da88f821218`.
- Retained source SHA-256:
  `7b6d1cf0a7b1bcd48c8e3917f0d62faf2b76105d1c1fca1ef07a8470ab32f10e`.
- Retained source Git blob:
  `b848624e32447ee41ad3cf4bd14b0b17aab6e320`.
- Final strict gate: 7 exact, 0 residual, 9 unwritten of 16 functions.
- `git diff --check` and the prohibited-construct scan pass.

No header, configuration, semantic ledger, parked entry, tool, Units, Vehicles,
Claude-active path, or other object was modified. Nothing was committed or
pushed from this isolated lane.
