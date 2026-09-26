# Worker C1 cards (COMMON-pool evidence packets, research only)

Snapshot for every measurement below: scratch/campaign/workers/C1/snap_20260926_011221 (HEAD 09f5208f,
source/config clean). Tools in tools/; derived data in data/.

## LAB1 - VC7 COMMON link semantics (card cards/LAB1_vc7_common_link_semantics.txt, written 01:22:03)
Outcome appended 2026-09-26 01:23:54 -0700.  Lab: scratch/campaign/workers/C1/lab_common (4 TUs, pinned CL 13.00.9254.1 +
xbox/bin/vc7/Link.Exe 7.00.9290, links a,b,c,m and m,c,b,a with /DEBUG /MAP).
- P1 CONFIRMED. Every COMMON variable's /MAP line reads Lib:Object = "<common>" (7/7 in both links). A
  linker map - including the Sept-2001 cachebeta map, were its data half recovered - cannot name the definer.
- P2 CONFIRMED. DIA2Dump -c attributes every COMMON contribution to "* Linker *"; the same module also
  contributes a 0x1C .rdata record (debug directory) and a .rdata$debug CodeView record - exactly January's
  two non-.bss "pooled" records (module 847 = "* Linker *" in cachebeta.pdb, verified with DIA2Dump -m).
- P3 PARTLY WRONG (direction). Runs ARE contiguous per object, and a symbol tentatively defined in two TUs
  (shared_ab in a.c and b.c) sits in the run of the FIRST-PROCESSED definer (a in link 1, b in link 2).
  But runs are laid out in REVERSE processing order (link a,b,c,m -> pool m,c,b,a), and inside a run the
  order is the reverse of the object's symbol-table order (a.obj symtab shared_ab,a_one,a_two -> pool
  a_two,a_one,shared_ab).
- P4 WRONG as stated, and that resolves the January puzzle. PDB module index is REVERSE command-line order
  (a,b,c,m -> m=1,c=2,b=3,a=4). So "pool ascending by module index" (Lane C's measured law) = "pool in
  reverse command-line order", and January's .text in DESCENDING module index = command-line order. Under
  this lab law a pooled record sits in the cluster of the definer with the HIGHEST module index among all
  of its tentative definers - it says nothing about other definers or about importers.
- P5 CONFIRMED. With /Zi, DIA2Dump -compiland a.obj lists only _use_a; a_one/a_two/shared_ab appear only in
  the global stream (-g). A PDB module stream cannot name a COMMON definer (HCEX's Halo globals are also
  "* Linker *" contributions: checked rasterizer_frame_statistics 0x1884060 and ai_globals 0x199025C).
- P6 CONFIRMED. c.obj's reference to a_one is UNDEF value 0; only a.obj's own symbol table carries COMMON
  (sec 0, value 4). January's 832 split objects carry ZERO COMMON-style symbols (obj_scan.py), so the
  split cannot distinguish a definer from an importer.
- Side fact: VC7 Link aligns COMMON by size (January: 1->1, 2->2, 4->4, 8->8, 10..16->16, >=24->32;
  83/77/46/21/8/5 records) and leaves unexplained 10-12 B holes in the lab pool; January's pool has 806 B
  of inter-record gaps. Not investigated further (no ownership content).

## LAB3 - initialised definition vs COMMON (card cards/LAB3_initialised_definition_beats_common.txt)
Outcome appended 2026-09-26 01:34:20 -0700.
- R1 CONFIRMED. With d.obj holding 'int a_one = 0;', the map places _a_one in d.obj (0002:00000000), not
  <common>; a.obj's COMMON a_one is resolved to it. So every record January pools had NO initialised
  (non-COMMON) definition in any linked object: all of its definitions were bare tentative definitions.
  Pooling does not say which TU(s) held them.
- R2 WRONG. A 40-byte COMMON a_two resolved SILENTLY (rc 0, no diagnostic) to e.obj's 20-byte initialised
  definition. VC7 Link does not police COMMON-vs-definition size here.
- Side fact: without /DEBUG (so /OPT:REF) unreferenced COMMONs (b_two, c_one) were DISCARDED. January keeps
  13 records that no January object references (collision_debug_radius/center, light_x/y/z, num_drawn,
  num_triangles, color32, global_(continuous_)damage_reference, __env_initialized, 2 PchSym), consistent with
  a NOREF (debug) link - and each of those exists only because some TU tentatively defined it without any
  January object referencing it, i.e. "definer = a referencer" (H_ref) is false for them by construction.

## LAB2 - library-member COMMON order (card cards/LAB2_library_member_common_order.txt, written 01:25:50)
Outcome appended 2026-09-26 01:39:57 -0700.  bc.lib = {b.obj, c.obj}; links 'a m bc.lib' and 'm a bc.lib'.
- Q1 WRONG. Library members' COMMONs do NOT go to the front: pool = [plain objects, reverse command line
  (m,a / a,m)] then [library members c, b]. The simple global push-front model (M2) is refuted.
- Q2 CONFIRMED. Modules: plain objects reverse command line (m=1,a=2 / a=1,m=2), then members in pull
  order (b=3,c=4) - yet the members' pool runs are c then b (DESCENDING module index). "Ascending module
  index" is therefore NOT a link-wide law; it describes plain command-line objects only.
- Q3 CONFIRMED in shape. January's pool = Halo plain-object segment (3..215) followed by per-library vendor
  groups: xapilib (216-224, incl. basedll PchSym), dsound (225, dsoundi PchSym), libcmt (226-242). The
  vendor tail's module indexes (0x1F0,0x1EF,0x1FF,0x214,0x006,0x1F9,0x1FC,0x002,0x309,0x324,0x2EF,0x30D,
  0x2E6,0x2F5) are neither ascending nor descending, so for library members the pool order is not a
  module-index oracle at all.  Consequence: the pool-order analysis (order_constraint.py) is restricted to
  the Halo segment and stays hypothesis-conditional.

## E1 - evidence census (no card: pure measurement, no prediction)
2026-09-26 01:39:57 -0700: obj_scan/decl_scan/lib_scan/vendor_identity/pdb_evidence/order_constraint/
lanec_wave/build_packets/canonical_placement_check run on the snapshot.  Results in COMMON_EVIDENCE.md.
- decl_scan's tentative set == obj_scan's COMMON set (67 == 67): the heuristic lexer validated.
- 0 of 832 January split objects carries a COMMON-style symbol (P6 at board scale).
- 27/27 vendor tail records have exactly one defining member per library (XDK-3911-era libs); all 27
  sizes equal January's; xapilib 505/505 and libcmt 402/405 comparable functions byte-identical to January's
  split objects (every function of every DEFINER member identical).
- HCEX: 143/238 found; 107 pooled there too ("* Linker *"), 28 declared-but-discarded (RVA 0), 7 CRT in
  obj\xbox\mbctype.obj, 1 Halo record defined in a compiland (debug_sound_reference_counts ->
  pc_sound_cache.obj).

## V1 - verdict rule (decided before build_packets.py was run; recorded here at close)
Rule: only classes that can OBSERVE a COMMON definer may PLACE (EC-LNK, EC-LIB + EC-MOD + size + build
identity, EC-PCH); a later-build observed definer (EC-HCEXDEF) may only CONSTRAIN; pool order (EC-POOL),
referencers, split form, our tree, publics, HCEX types and the map never change a verdict (Q10 ruling).
Outcome 2026-09-26 01:46:56 -0700:
- PLACED 29 (27 vendor 1,125 B + 2 linker 88 B); CONSTRAINED 1 (Halo, 1 B, _debug_sound_reference_counts);
  UNPLACED 212 (Halo, 1,270,476 B).  Total 242 / 1,271,690 B (contribution sizes).
- Failed/corrected along the way: first vendor-identity pass masked only one side's relocation fields and
  showed 11 false differences (fixed: mask either side's); first packet pass attributed libcmt records to
  libc.lib (alphabetical pick; fixed: prefer the identity-checked member); first order pass used a greedy
  that cascaded 157 false conflicts (replaced by a min-violation DP: K*=2); 3 REL32 relocations from
  libcmt fflush/getws/putws to _ai_debug were csplit artefacts and are excluded from referencer sets.
- COMMON_EVIDENCE.md said "66 of 67" canonical COMMON placements consistent under H_order+H_ref; the
  recount is 64 (2 header records with EXCLUDED definers + 1 shadowed-only); corrected before hand-off.
