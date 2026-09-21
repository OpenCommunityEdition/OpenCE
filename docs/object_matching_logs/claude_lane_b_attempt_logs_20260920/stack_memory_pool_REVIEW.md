# Independent review: source/memory/stack_memory_pool (Lane B)

Verdict: LAND_PARTIAL - land `review/landing.c`, NOT `candidate.c`.

## Re-gate from scratch (candidate.c)
- unitcheck: baseline 31/2/0 -> candidate 33/0/0; gains=2 regressions=0 other=0.
- gate --fn: allocate EXACT 640, resize_block EXACT 192, dispose_block EXACT 160, dispose_handle EXACT 128,
  dispose_pointer EXACT 176. alndiff both claimed functions: no normalized instruction differences (231/231).
- guard `--forbid-emitted-symbol _point_from_line3d`: passed. newsyms: defines 0 new / drops 0, OWNERSHIP PASS.
- fake_match_scan: 0 leads on candidate and on real source (no new findings).
- Line endings: real 1198 CRLF / 0 LF; candidate 1201 CRLF / 0 LF. No header/config edits. No parked entries for the unit.
- Non-code section multiset (name,size,sha,relocs) base.obj == cand.obj == landing.obj (review/seccmp.py): no data movement.

## Evidence re-derived independently (review/od_*.txt regenerated, identical to worker dumps)
- allocate /Od 0x6e9da0: `mov [ebp-4],0` is the first statement, before the 0x342 assert; one return (`mov eax,[ebp-4]`);
  positive guard jbe/ja/jae -> else arm line 0x3A4; `[ebp-0x10]=0` then `[ebp-0x18]=0` after the guard; `add eax,0x20;
  mov [ebp+0xc],eax` and a `div 4` loop on the size PARAMETER; local [ebp-8] assigned from free_space_at_end twice and
  compared three times; `>= || free_space` with else arm 0x39F; `cmp [pool+0x10],-1; je` wraps the insertion.
- January: `xor esi,esi` @0xc precedes the assert call @0x2c and is returned by `mov eax,esi` @0x275 (invalid-size path);
  NULL stores `[ebp-4]`,`[ebp-8]` @0x60/0x63; assert lines bottom-ordered 0x39F.. 0x3A4. => `block = NULL` at declaration
  is DOUBLE-ATTESTED. Strip S2 (assign NULL after the assert instead): residual [sha] -> the position is real, and attested.
- dispose_block: /Od resize 0x6eab36 `push [ebp+0xc]` (reference) then `push [ebp+8]` (pool); callee 0x6ea410 passes
  [ebp+8],[ebp+0xc] to valid_block(pool, reference). (pool, block) order attested. All 3 call sites updated; no other users.
- resize_block result initialiser: /Od `mov [ebp-4],0` first; January `xor eax,eax` @0x7 returned on new_size==0. Attested.

## Strip tests (review/mkvariants.py, all binary-safe CRLF)
| variant | change vs candidate | allocate |
|---|---|---|
| strip_s1a | REMOVE pointer local `last_block`; `else if (block > pool->last_block)`; /Od store order previous,next | EXACT |
| strip_s1b | same, but candidate store order next,previous | residual [sha] |
| strip_s2 | `block = NULL` moved after first assert | residual [sha] |
| strip_s3 | inner locals in /Od frame order (free_space_at_end_of_pool first) | EXACT (inert; not applied) |

FINDING: `struct stack_memory_pool_block *last_block; last_block = pool->last_block;` (kept by the worker as "untouched
insertion body") is a pointer local the /Od build does NOT have (frame slots: -4 block, -8 free_space_at_end_of_pool,
-0x10 free_space_in_pool_previous, -0x18 free_space, -0x1c block_index; 0x6ea070..0x6ea0a6 reads `[pool+0x30]` directly,
stores previous THEN next, `else if` topology). The plain /Od spelling compiles EXACT, so by the strip rule the plain
spelling must land. `review/landing.c` = candidate.c with only that hunk replaced (review/landing_vs_candidate.diff).

## landing.c verification
unitcheck gains=2 regressions=0 other=0 (33/33); guard passed; newsyms 0/0 PASS; fake_match_scan 0 leads;
1195 CRLF / 0 LF, ASCII, no BOM.

## Pre-existing debt seen (NOT introduced by this candidate, not blocking)
- `block->handle = (void **)file; block->lock_count = line;` - the .c-local struct fields at +0x10/+0x14 are misnamed
  (they hold the allocation file/line; `handle`/`lock_count` have no other user). Follow-up: rename/retype, drop the cast.
- /Od pushes memory_block_set_size_and_index args as (block, size, index); ours is (index, block, size). Baseline-exact.
- File-wide hand-expanded `display_assert(..., FALSE)` warnings (14 sites) predate the candidate; it only moves two.
