# Profile frame-value getter: cross-build source authentication (2026-09-19)

## Result

`source/cseries/profile.c::_profile_frame_get_value` is now strict exact:

- January/candidate size: **928 / 928 meaningful and padded bytes**
- January/candidate relocations: **50 / 50**, zero differing rows
- normalized SHA-256: `34007f52073a5931791a093800b82b1569907d4fe1c5f5dd5333df7ecaacf121`
- whole-unit isolated gate: **43 exact / 1 residual / 0 unwritten**
- inherited exact Profile functions lost: **0**
- forbidden `_point_from_line3d` code definitions: **0**

This reopens the park using the exact evidence class it requested: authentic
same-revision source/compiler context. It is not a register, lifetime, or
statement-order search.

## Prior hold

The natural reconstruction was already size/relocation exact. Its only
instruction-stream residual was the `_profile_frame_value_render0_3np` loop.
January tests `window_ids[window_index]`, executes the window-total add on both
paths, and increments `player_window_count` only on the true path.

VC7 experiments in
`docs/object_matching_logs/profile_obj_opus5_150k_w2_20260914.md` established
that the release instruction order is emitted only when the source writes the
common add in both arms. The candidate was held under R2 because that compiler
signature alone did not prove Bungie's source used the two-arm spelling. The
later w3 ledger explicitly asked for January-side corroboration before reopening
it.

## Decisive same-revision evidence

The first-party symbols executable used here is:

```text
C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\symbol-build-h1-tags-20260906\halo_cache_symbols.exe
SHA-256 740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55
```

It contains the private frame-value strings, including `render0_3np`, and the
same assertion strings used by January's getter. The `/Od` function at
`0x00554900..0x00554e21` is therefore independently identified as
`profile_frame_get_value`; the `render0_3np` case is
`0x00554cc7..0x00554d5a`.

The unoptimized code preserves two distinct source arms:

```text
00554d0d  test eax,eax
00554d0f  je   00554d3b

00554d11  ... increment player_window_count ...
00554d1c  ... frame->windows[window_index].total ...
00554d2b  addss xmm0,[ecx+eax+0xe48]
00554d34  movss [ebp-8],xmm0
00554d39  jmp  00554d58

00554d3b  ... frame->windows[window_index].total ...
00554d4a  addss xmm0,[ecx+eax+0xe48]
00554d53  movss [ebp-8],xmm0
```

Because this is an unoptimized Bungie symbols build, the duplicate additions
cannot be an optimizer-created common expression. They directly authenticate
the original two-arm source shape. The January optimized object then exhibits
VC7's already-measured common-arm hoist: `test; lea; fadd; je`.

The saved disassembly is
`scratch/profile_frame_get_value_symbol_build.disasm.txt`, SHA-256
`146f74b55341f06b2b57c7c0615c0fed3ebcee4768a08c9b571ad7d8f445c91b`.

## Minimal source correction

Only the residual loop changed:

```c
if (frame->window_ids[window_index])
{
	value += frame->windows[window_index].total;
	player_window_count++;
}
else
{
	value += frame->windows[window_index].total;
}
```

Each reachable path performs exactly one accumulation. There is no dead code,
fake dependency, lifetime steering, undefined behavior, inline assembly, raw
offset, or new owner. The spelling is now source-backed rather than merely
compiler-shape-compatible.

## Verification

```text
python tools/campaign/gate.py source/cseries/profile --all \
  --out scratch/profile-crossbuild-authenticated.obj \
  --forbid-emitted-symbol _point_from_line3d

== emitted-symbol guard passed (1 forbidden names checked)
EXACT       928  _profile_frame_get_value
== exact 43  residual 1  unwritten 0  (of 44 listed)
```

```text
python tools/campaign/relocdiff.py source/cseries/profile \
  _profile_frame_get_value scratch/profile-crossbuild-authenticated.obj \
  --only-differences --count-by-target

shape: size 928/928, relocations 50/50, sha equal
summary: 50 target, 50 candidate, 0 differing row(s)
```

`alndiff.py` reports equal 356-instruction streams; its displayed replacements
are only equivalent literal-owner classifications (`symbol` versus
`defined-noncode`) and the strict gate/relocation comparator resolves them as
identical. `tools/fake_match_scan.py source/cseries/profile.c` reports only the
seven inherited `QUERY_TIMEBASE`/RDTSC assembly leads and no new finding.

The candidate object is
`scratch/profile-crossbuild-authenticated.obj`, SHA-256
`608e6b044f124e4bc206961a2365f4d6318527009a07588e05d04b703cf588e9`.

## Integration note

`config/parked.json` was already modified by the concurrent integration
campaign, so this work deliberately does not edit it. The integrator should
remove only the stale `_profile_frame_get_value` park after rebuilding the
canonical report. `_compare_profile_sections` remains parked and unchanged.
