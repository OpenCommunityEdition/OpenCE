# E1 owner questions (prepared 2026-09-26; nothing here is landed or landable without the ruling)

Base: worktree HEAD 26684ca8 (E1 units byte-identical to published canonical a8854940). Every packet below was
re-verified at this HEAD with `scratch/campaign/review_patch.py` (battery/FINAL_review_and_keyed_diffs.txt).
Rules cited by name from HALO_HOUSE_RULES_20260926.md.

## E1-Q1. Admit `source/ai/path_structure_bsp` (6/6) with the /Od loop cleanup? (object +1, 0 code/data bytes)
- Patch: `path_structure_bsp.patch` (sha256 dca5c196...). It replaces `_structure_test_ray2d`'s unattested
  `goto continue_from_surface` and its byte-flags local with the first-party /Od topology (halo_cache_symbols.exe
  0x4cfe60: `long next_surface_index = NONE;` at the loop top, per-arm block-scoped `passable`/`collision_surface`,
  `if (next_surface_index == NONE) break;`). The object is byte-identical to build/base (keyed_diff 0/0/0).
- Battery (battery/path_structure_bsp.txt), HEAD and candidate: object_audit PASS (13 January symbols, 0 differ);
  pdb_storage 0; surplus_identity 10 helper COMDATs all IDENTICAL to January's selected copies; provider_link PASS
  both orders (14 rows; the old actor_combat `_cross_product2d` / action_vehicle `_distance_squared2d` NODUP blockers
  are gone since 3ee32a5c / 931ed8dc); protoscan 0; /W3 identical (header-only C4146/C4244, no C4013); fake scan 0;
  no open admission rejection; census: 0 code / 0 data remaining.
- Disclosures the owner must weigh (source review):
  1. Unguarded neighbour reads. ray2d reads `pathfinding_surfaces[surface_result.enter_surface_index]` (and exit)
     and line2d reads `pathfinding_surfaces[edge->surface_indices[!on_right_side]]` before any NONE test. January
     does this (ray2d +0x63/+0xec load before the -1 compares at +0xcc/+0x159) and so does the later /Od build
     (0x4cfe60 +0x98/+0x168; 0x4cee00 +0x330..+0x339) - the P1c spelling shows January's `cmp -1` is the /Od
     `next_surface_index == NONE` not-found flag, not a guard. The index can only be NONE for an edge with no
     neighbour surface (an open collision BSP); collision_surface_test_line2d's own NONE initialisers stay unread
     because enter_t/exit_t stay at REAL_MIN/REAL_MAX. First-party corroboration of the closed-BSP invariant: the
     2020 tool build asserts `edge->surface_indices[0/1]>=0 && <bsp->surfaces.count` in
     `tool\import_collision_bsp\reduce_collision_bsp.c` (tool_symbols.exe strings 0x876630/0x876690). On an open BSP
     the read is one byte before the tag block (formally out of bounds; line2d would then assert in
     TAG_BLOCK_GET_ELEMENT). Earlier notes (20260830 defined-behaviour boundary; laws_w2 R12) classed the historical
     body inadmissible for this read; the functions are nevertheless credited exact in canonical today.
  2. Nine 3D->2D `(real_point2d const *)` view casts (line2d 5, pill2d 3, incl. `&point_in_surface`): every site is
     /Od-attested (card P2 lists the instructions), compatible prefix, and byte-inert (strip test: object identical,
     +9 C4133 only). The line2d vertex casts are the owner's 2026-09-21 per-site admission (3ba2eb91).
  3. TU-local `_collision_surface_breakable_bit` / `_pathfinding_surface_*_bit` enums duplicated in 9 files (no
     owner-header enum exists); board-wide pattern, not specific to this object.
- Question: may path_structure_bsp land `path_structure_bsp.patch` and flip to Matching, treating item 1 as the
  program's closed-BSP contract (no BUG comment) - or must item 1 first receive an original-bug ruling (then: BUG
  comments at the three reads, byte-inert, and which layout assertion would the owner accept)?

## E1-Q2. `_connected_geometry_find_or_add_edge` (240 padded / 240 meaningful): uninitialised `direction`
- Patch: `owner/connected_geometry_find_or_add_edge_OWNER_Q.patch` (sha256 bbb2bb97...): Lane D's /Od topology (for
  loop, else-if, block-scoped edge pointers, name `direction`, single ternary return) with the project `SET_FLAG`
  macro and the existing named bit (card C1), and `boolean direction;` WITHOUT initializer plus the BUG comment.
  review_patch: 1 gain, 0 losses, 0 added/removed; object 8/10 (vertex and coplanar stay residual - no completion).
- January evidence (quoted): `+0x47 mov byte ptr [ebp-1],1` (forward match), `+0x4d mov byte ptr [ebp-1],0`
  (reverse match), `+0x67 mov byte ptr [ebp-1],1` (new edge), read once at `+0xbe mov al,byte ptr [ebp-1]`; nothing
  writes it on entry. The later /Od+/RTC build names it: `_RTC_UninitUse("direction")` (Lane D, /Od 0x8bb620).
- Reachability (NEW): the read without a write needs `geometry->edges.count < 0`. dynamic_array_delete stores
  `count = NONE`, so only a deleted array qualifies, and on that path January first calls
  dynamic_array_get_element (+0x9b), whose asserts (array.c 0x7E element_size>0, 0x80 count>=0; January
  `_dynamic_array_get_element` strict exact) halt before +0xbe. The read is unreachable in January's execution
  (same class as the approved Q6 packet: unreachable in defined execution).
- Consequences: no new symbols, no header change, no data change; one inherited-style BUG comment.
- Fallback without a YES: Lane D's safe `boolean direction = TRUE;` form is zero credit (256 B, only the +0x10
  initializer store differs): `owner/connected_geometry_find_or_add_edge_SAFE_FALLBACK_zero_credit.patch`.

## E1-Q3. `_connected_geometry_find_or_add_vertex` (192 padded / 189 meaningful): `realcmp_epsilon`
- Re-posed without new first-party evidence. Lane D's candidate (re-verified at HEAD: 9/1 together with Q2) needs
  `#define realcmp_epsilon(a, b, epsilon) (fabs((a)-(b))<(epsilon))` (the macro text in sound_dsound_xbox.c:61,
  itself an inferred 2026-09-13 macro). Byte fact: January's y/z subtractions evaluate the left operand before the
  getter call, which VC7 does only when both operands are parenthesised (Lane D g13/g15); the plain fabs spelling is
  176 B (strip test fails). I searched every first-party binary (cachebeta.exe, halo_cache/tool/sapien/guerilla/
  halo_tag _symbols.exe) for an epsilon-taking realcmp string: none (only `realcmp(...)` and
  `assert_valid_realcmp`). Options: (a) TU-local macro (second copy; runs against e942f338's consolidation); (b)
  owner-header `realcmp_epsilon` in real_math.h (structural packet s3: 0 gained / 0 regressions on 273 consumers).
  Held unless the owner rules; the object cannot complete anyway (`_triangle_coplanar` remains a scheduling tie).
  Packet: `owner/connected_geometry_edge_plus_vertex_realcmp_epsilon_OWNER_Q.patch` (sha256 778b2850...) = Lane D's
  `candidate_ruling_direction_realcmp.c` unchanged (it carries Lane D's edge spelling, LONG_MIN/LONG_MAX tail and the
  enum removal; with a YES on Q2 the lead should re-base its vertex hunk onto the C1 edge packet).

## E1-Q4. `_dead_camera_update` (1,248 / 1,235): refreshed packet (owner queue #9), completes dead_camera
- Patch: `owner/dead_camera_update_OWNER_Q.patch` (= fifty-objects 01_dead_camera_update_bug.patch, sha256
  7cb8cbba...). Re-verified at HEAD: 4/4 exact, review_patch 1 gain 0 losses; object_audit PASS; pdb_storage 0;
  surplus 8 COMDATs identical; provider_link PASS; /W3 identical; fake scan 0 (battery/dead_camera_ownerQ.txt).
- January (quoted): `+0x14e call _player_get_next_player_with_a_unit; +0x159 cmp eax,-1; +0x15f je 0x176;
  +0x176 mov eax,dword ptr [ebp+0x10]` - the NONE path loads the `result` parameter home (only read before, at
  +0x20/+0x3b) as the unit index. The later /Od build initialises next_unit_index = NONE (the fix).
- New since the queue entry: (i) Codex reachability audit (dead_camera_small_object_reaudit_20260924.md): every valid
  camera starts with a valid player index and player_get_next_player_with_a_unit returns the old index when none is
  found, so the NONE branch is unreachable in the valid camera lifecycle; (ii) Q6 (2026-09-26) approved an
  unreachable-in-defined-execution uninitialised result. AGAINST: the owner's explicit 2026-09-20 exclusion of this
  donor (astra rejected hypotheses item 8) and the Q6 ruling's own limit ("not permission for reachable
  uninitialised reads"); the path is reachable for a corrupt/out-of-contract camera. Held unless ruled.

## E1-Q5. hardware_geometry MoveResourceMemory (NO new evidence; the 4d1ebf17 hold stays)
- The name `_D3DVertexBuffer_MoveResourceMemory@8` is authenticated by the 2001-09-25 map (Codex doc); call position
  and D3DMEM argument remain unattested (no executable trace; static __forceinline). Not re-asked.
- Independent of it, `rasterizer_xbox_hardware_geometry.patch` + lead proposal `proposals/symbols_hardware_geometry_H1.patch`
  remove four hand-written Unlock stubs (placeholder `code_` names) that duplicate genuine XDK header emission
  (zero credit; object_audit FAIL(7) -> PASS; pdb 13 -> 2). The object stays NonMatching until Q5 is ruled.

## Lead choice (not an owner hold): periodic builder helper spelling
- `periodic_functions.patch` (primary, P3): the /Od term-1 spelling only; builder keeps `(real)cos` (January's
  build_table REFUTES cosine()/sine() by bytes, so direct cos is the within-TU January style).
- `periodic_functions_ALT_cosine.patch` (P3c): also switches the builder to real_math.h `cosine()` as the /Od build
  does; emits ONE new `_cosine` COMDAT (16 B) identical to January's selected actor_combat copy, provider link PASS
  both orders (rule "The old blanket _point_from_line3d emission ban has a narrow folded-inline exception" conditions
  met: exact caller, identical copy, links). Pick one.
