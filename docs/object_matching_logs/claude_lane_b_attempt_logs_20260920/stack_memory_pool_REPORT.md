# stack_memory_pool worker log (Lane B)

Baseline gate: == exact 31  residual 2  unwritten 0  (of 33 listed)

## _stack_memory_pool_allocate (640 B) - CLOSED, shape 1 (first compile)

January (aln_alloc_full.txt): `xor esi,esi` at 0xc BEFORE the first assert, returned by `mov eax,esi` at 0x275
(invalid-size path) = a NULL-initialised result VARIABLE, not a zero pin; a second zero web `xor eax,eax` at 0x3b
feeds `cmp ebx,eax`, the two NULL stores [ebp-4]/[ebp-8] and `push eax` (FALSE) at 0x25c. Assert line order
0x35F,0x370,0x37D,0x394,0x39F,0x3A4 = both failure diagnostics are bottom else-arms. frame 0x8, 6 rets both sides.
/Od (od_allocate.txt, fn 0x6e9da0): `mov [ebp-4],0` (block = NULL) is the FIRST statement, before the assert; ONE
return (`mov eax,[ebp-4]` at 0x6ea149); positive guard with else-arm "invalid size"; `size` parameter modified in
place (`add eax,0x20; mov [ebp+0xc],eax`, `div 4` loop); local [ebp-8] holds free_space_at_end_of_pool, tested 3
times (`<`, `<`, `>= || free_space`), else-arm "allocation ... failed"; `if (next_block_index != NONE) {...}` wraps
the insertion; RTC name free_space_in_pool_previous.
Change: skeleton rewritten to that shape (block = NULL at declaration, single `return block;`, nested positive
guards with else asserts, in-place `size`, free_space_at_end_of_pool local). Insertion body untouched.
Gate before: residual    640  _stack_memory_pool_allocate  [sha]
Gate after : EXACT       640  _stack_memory_pool_allocate      unitcheck gains=1 regressions=0
Shape 2 (spelling check): `while (size%4)` (the /Od div form) instead of `while (size&3)`: also EXACT. Kept %4.
No decoration in the change - nothing to strip. candidate.c = try_2.c.

## _stack_memory_pool_resize_block (192 B) - CLOSED, shape 1 (try_3.c)

January (aln_resize_full.txt): no frame, 3 rets, reloc multiset 13/13 identical; only gap = `reference` reloaded
from its home at 0x69 (`mov eax,[ebp+8]`) and 0x8a (`mov esi,[ebp+8]`) where ours kept one ESI web (184 vs 180 B).
/Od (od_6eaa50.txt = resize_block, od_6ea410.txt = dispose_block): body topology identical to our source, BUT the
dispose call is `push [ebp+0xc] (reference); push [ebp+8] (pool); call 0x6ea410` and the callee reads pool at
[ebp+8], block at [ebp+0xc] => first-party signature is dispose_block(pool, reference). Ours had (block, pool).
Change: dispose_block prototype + definition parameter ORDER to (pool, block); 3 call sites updated. No body edits.
Gate before: residual    192  _stack_memory_pool_resize_block  [sha]
Gate after : EXACT       192  _stack_memory_pool_resize_block
unitcheck: gains=2 regressions=0 (dispose_block, dispose_handle, dispose_pointer stay EXACT: ESI=block/EDI=pool
convention unchanged in the callee, the caller's argument-evaluation order is what moved).
Isolation (try_4.c = real source + ONLY the parameter-order change): resize_block EXACT, allocate still residual
=> the two levers are independent.

## Final (candidate.c = try_3.c)
gate --all --forbid-emitted-symbol _point_from_line3d: guard passed; == exact 33  residual 0  unwritten 0  (of 33 listed)
newsyms: newly DEFINES 0, no longer defines 0, OWNERSHIP PASS. fake_match_scan: 0 review leads. CRLF preserved.
