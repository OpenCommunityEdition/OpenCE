# `stack_memory_pool.obj` retention-source audit (2026-09-01)

## Result

The inherited object remains **31 exact / 2 residual / 0 unwritten** with
100% target-owned data, but it is not newly fuzzy-parked or admitted. The
translation unit still contains a synthetic `if (FALSE)` block in
`stack_memory_pool_reset` that names nearly every private helper solely to
change VC7's call-count and inlining decisions. That source predates the
current no-fake-source rule and was introduced as a temporary scaffold while
the real allocator callers were unwritten. Those callers have now landed, so
the scaffold was re-audited rather than silently grandfathered.

Removing the complete block is runtime-code neutral but changes compiler
context dramatically:

| Candidate | Exact | Residual | Unwritten |
|---|---:|---:|---:|
| inherited `/O2` with retention block | 31 | 2 | 0 |
| ordinary `/O2` without retention block | 16 | 14 | 3 |
| `/O2 /Ob2` without retention block | 16 | 14 | 3 |
| `/O2 /Ob1` without retention block | 22 | 8 | 3 |
| `/O2 /Ob0` without retention block | 22 | 8 | 3 |

The three omitted private sections are `memory_block_unlock`,
`stack_memory_pool_new_block`, and `stack_memory_pool_new_block_clear`. The
other losses are caller/helper inlining and lifetime changes, not lost
allocator logic. The removal candidate was inverse-restored after measurement;
production source is unchanged in this lane.

## Evidence boundary

The January object proves that the private sections existed and that its
callers used the non-inlined forms, but it does not prove this exhaustive dead
call block. The block's own comment and Git provenance identify it as a
reconstruction scaffold, and `tools/fake_match_scan.py` correctly reports its
fixed-false condition. Replacing it with `__declspec(noinline)`, an inline
pragma, fake function-address uses, another dead branch, or an object-specific
compiler flag would merely exchange one unsupported codegen control for
another.

The 82-experiment full-admission ledger, the Stian/Pastudan/HaloCEA donor
census, and the later exact recovery of
`stack_memory_pool_find_space_between_blocks` were all reviewed. They remain
good evidence for the 31 emitted function bodies and for the two genuine
residual classifications, but they do not authenticate the missing
translation-unit call-count context.

## Disposition

- Do not delete the scaffold in canonical merely to make the source scan clean;
  that would discard fifteen strict-exact sections without recovering the
  original context.
- Do not grant parked credit to `stack_memory_pool_allocate` or
  `stack_memory_pool_resize_block` while the unit still depends on the
  unsupported retention block.
- Keep the object `NonMatching` and reopen from authoritative January source,
  a PDB/local record that explains the helper references, or a compatible
  compiler/build record that naturally reproduces the non-inline schedule.

For repeatable diagnostics, `tools/campaign/gate.py` now accepts repeatable
`--cflag` options. The option is deliberately documented as an audit aid, not
as admission evidence.
