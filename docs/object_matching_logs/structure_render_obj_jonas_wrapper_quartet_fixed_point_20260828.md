# `structure_render.obj` Jonas wrapper-quartet fixed point

## Result

This bounded one-emission wave starts from verified integration commit
`c8c40b2ceaaee631f11abbed03447257c83f9a36`. The frozen quartet was compiled
once as ordinary typed C. None was strict, so every candidate body was pruned
immediately without tuning. `structure_render.obj` remains 1/19 exact and
`NonMatching`; the retained source is byte-identical to the starting source.

| Candidate | Target padded bytes / relocations | One-shot disposition |
| --- | ---: | --- |
| `_structure_render_reflection_mirrors` | 64 / 8 | residual; pruned |
| `_structure_render_reflection_lightmap_masks` | 80 / 10 | residual; pruned |
| `_structure_render_diffuse_texture` | 128 / 16 | residual; pruned |
| `_structure_render_reflections` | 128 / 16 | residual; pruned |

No exact function or meaningful byte was gained.

## Evidence and frozen candidate

The January split object is SHA-256
`113539484a7eaacc6eafa639163888842e5ad2e4d3ca181ee2d2fd659c7dc503`.
The sole candidate object is SHA-256
`f5ebfabf72537d003ecfb6ca9879d33a71bc0e0574160c54884634761208c734`.
The existing fog-offset ledger authenticates the January executable, compiler,
target object, direct `bss_004c0cd0` owner, render layout, and retained 1/19
baseline. Its committed source blob is
`95cd4a11785342bd098a4983c5a3aac99bed5138`.

All prior `structure_render` ledger and worktree evidence was read before the
candidate. The only previous ledger is
`structure_render_obj_jonas_fog_offset_leaf_20260821.md`; it records no attempt
on this quartet. The prior fog-offset worktree was inspected read-only and had
no relevant uncommitted work. Canonical `CLAUDE.md`, `AGENTS.md`,
`trees_obj_codex_checkpoint.md`, and campaign documentation governed this
wave.

The clean HCEA semantic oracle is commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Its four donor blobs are:

- diffuse texture: `dee8710b0fa94b67234e17f193687dd7c16bfc5a`;
- reflection lightmap masks: `8bad0e9751330c1cac7bbdca6f08b0af64564d6b`;
- reflection mirrors: `43f7c1315b61389f2e0216d1ea348a296d4f0152`;
- reflections: `f2ce8c6676152d0448ce7051decf881a873883b2`.

HCEA's render-window caller blob is
`e1b4d6fb080e207aa404ef884a733b4557c4ddf5a` and authenticates all four as
ordinary `void(void)` scene stages. Pinned Stian commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf` and pinned Pastudan commit
`918af885935ec470a31256ecce9a977b12b01f80` contain none of the four symbols;
they are negative provenance, not substitute donors.

The frozen C candidate used the public render and rasterizer types, the typed
render-global fields, the typed direct structure-render global prefix, and the
HCEA begin/pass/end semantics. It used explicit `void`, one parameter per
line, explicit terminal returns, and no forcing device or forbidden construct.

## Why the fixed point is closed

For mirrors and lightmap masks, candidate and target have identical padded
extent and the same surrounding control flow. The strict difference is the
call into private `_code_00184fe0`: January passes the environment-surface
address in EAX and six remaining arguments on the stack, cleaning `0x18`
bytes. Ordinary typed cdecl emits seven stack arguments and cleans `0x1c`.
Reproducing January requires expressing a private register ABI, which this
campaign forbids. The candidate was therefore pruned rather than tuned.

Diffuse texture and reflections contain the same private pass ABI and also
wrap their cores with `_profile_global_enable`, a TU-private profile-section
owner, and `_profile_enter_private` / `_profile_exit_private`. The natural
HCEA donors omit those January-private owners. Candidate output was 64 bytes
and eight relocations versus each 128-byte, 16-relocation target. Introducing
or borrowing private profile storage would violate the ownership and private
ABI boundary. Both candidates were pruned.

There is no legal donor/target reconstruction under the stated ordinary-C,
typed-public-ABI and ownership rules. This is an evidence-backed fixed point,
not a request for spelling, scheduling, flag, declaration, or layout retries.

## Ownership and validation

After pruning, `source/structures/structure_render.c` has no diff. The only
tracked change is this additive ledger. No storage, header, configuration,
semantic exception, parked record, completion label, Units, Vehicles, or
Claude-active path changed. The final normal rebuild restores the baseline
object and the strict gate must report the existing fog-offset leaf exact,
zero residual, and eighteen unwritten bodies. No commit or push is made.
