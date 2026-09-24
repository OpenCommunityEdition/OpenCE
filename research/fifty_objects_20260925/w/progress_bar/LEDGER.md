# progress_bar LEDGER (claude-fifty-objects-20260925)
- start: slug dir was empty on resume; starting fresh
## Baseline (2026-09-23 resume run)
- gate --all on production: 53 exact / 1 residual (_draw_gravy_layer 336 vs 352, sha); 0 unwritten.
- Prior ledgers read: progress_bar_obj_opus5_150k_w1_20260914.md, progress_bar_obj_opus5_next150_n1_20260915.md,
  base_layer_reconciliation_20260907.md. Recorded negatives (NOT to repeat as-is): S1 named corner locals after
  generate, S3 pointer alias, gA nested-block locals, gC two reused locals, gD whole-aggregate copy, gH static inline vertex helper.
- /Od build: progress bar replaced ("old loading progress screen has been replaced with glowy halo gravy", fn 0x666670) -> no /Od readout.
- HCEX.pdb: draw_layer_int / generate_gravy_rect / progress_bar_draw absent -> no HCEX evidence.
- January atlas name: draw_layer_int (0xd1770).
- alndiff: January loads each corner into scratch (ecx/edx), copies to carrier (ebx/eax/edi) and stores a HOME
  ([ebp-8]=y0,[ebp-4]=x0,[ebp-0xc]=x1,[ebp+0xc]=y1); x0 reloaded from home [ebp-4] at vertex 4. Ours loads rect.* directly
  into the carrier (ebx/edi) or scratch, no homes, x0 re-read from rect [ebp-0x1c].
- P1 s1 (re-measure of recorded S1: named x0,y0,x1,y1 = rect.* after generate): byte-identical to base (confirms record).
- FINDING: orphan global helper gen_mask_coord(x,y,mask) = SetVertexData2f(10, x-(m*768-64-64), y*0.033333335f)
  reproduces EVERY TEXCOORD1 of draw_layer_int with (x,y) in {(0,0),(640,0),(640,480),(0,480)} (480*0.033333335f folds to 16.f).
  It has no caller in our source nor any call reloc in January -> January call sites were auto-inlined.
- P2 g1 (prototype gen_mask_coord in the .c prototype block + 4 gen_mask_coord(x,y,progress) calls): VC7 inlines them;
  bytes IDENTICAL to base (mask part inert, corners still forwarded). Authentic-looking but no byte change.
- P3 t1 (corner vars assigned inside the v1..v3 call args) and P4 t2 (x0/y0 assigned just before v1, x1 before v2, y1 before v3):
  both 121 insns, identical family to base (VC7 remats the variables from rect memory). NEGATIVE.
- LAB l1 (scratch/w/progress_bar/lab/l1.c: same body, non-static, generate_gravy_rect only an extern prototype):
  VC7 re-reads every rect field at every vertex (NO CSE across calls). => in the real TU the cross-call CSE of rect.y0/x1/y1
  exists only because generate_gravy_rect (defined in the TU) is known not to capture &rect (IPA). January also CSEs, so
  January also had that IPA knowledge; the difference is remat-from-rect vs separate spill home.
- object_audit on production obj: all sections ok except _draw_gravy_layer; 24 symbol rows DIFF = D3D inline copies storage
  2 (January split) vs 3 (ours static) - systemic, not this function; plus candidate-only .rdata literal surplus.
- P5 u1_full (gravy_rectangle2d as union {real n[4]; struct{x0,y0,x1,y1};} like real_math's rectangle unions): 121 insns, same as base. NEGATIVE.
- LAB l2/l3: generate_gravy_rect defined in the lab file (before OR after draw) -> cross-call CSE appears, size 336 = production.
  Confirms the IPA no-escape mechanism; definition order inert.
- January generate_gravy_rect assert compares [rect+8] vs [rect+0] for "rect->x0 < rect->x1" -> layout {x0,y0,x1,y1} proven (NOT real_rectangle2d {x0,x1,y0,y1}).
- LAB harness scratch/w/progress_bar/labv.py (lab/head*.c + <v>.body + tail.c, static draw + 2 callers; v0 reproduces production bytes EXACTLY).
  Lab negatives (IPA present): va dead store to rect at end (inert), vc `real const *corners=&rect.x0` (escape: no CSE, 352 but no homes),
  vh/vi/vj direct D3DDevice_* calls (inert), ve rect in nested block (inert), vt2 (inert).
- **KEY LAB RESULT**: with generate_gravy_rect only extern (no IPA no-escape info), the t2 body (corner variables assigned at first use)
  reproduces January's signature: load into scratch, `mov eax,edx; push eax` carrier copy, home stores right before the vertex call.
  (xvt2: 368 B, frame 0x1c). S1 body (assign after generate) without IPA -> frame 0x2c, loads right after generate (not January).
  => hypothesis: January had NO no-escape knowledge for generate_gravy_rect at the draw call and corner vars assigned at use.
- LAB xgm/gm: t2 body + the orphan gen_mask_coord inlined for TEXCOORD1: identical to vt2/xvt2 respectively (mask form inert for allocation).
- xvt2 vs January at v1: identical shape except register choice: January y0 carrier = ebx (kept to v2), 640-mask HOMED ([ebp+0xc] reload);
  xvt2: 640-mask in ebx, y0/x1/y1 homed. => remaining gap in the no-IPA model is allocation priority of corner vars vs 640-mask.
- LAB IPA mechanism: gesc (generate passes rect to an extern) and gext (to_screen extern) -> NO CSE in draw; so VC7 uses a
  body-derived no-capture summary of generate (transitively through to_screen). to_screen defined after generate or after draw,
  K&R prototype, prototype+definition after draw: CSE persists (VC7 defers callers). dq1 quad helper: not inlined (not January).
- LAB cast1/cast2 (cast on &rect at generate call): CSE persists. Board scan (vwscan.py) for the V/W+home pattern: draw_gravy_layer is unique (other hits unrelated).
- LAB IPA-world forms f5 (incremental real_point2d vertex), f6 (p0/p1 point copies), ds1 (dead stores to rect between vertices):
  all forward/remat to base (no homes). IPA world has no natural form giving homes.
- LAB more IPA-killer candidates, ALL inert (CSE persists): arr1/arr2 (real rect[4] + cast), m1 (prototype `void *rect`),
  m2 (prototype before struct definitions = prototype-scope tag), ti (__inline to_screen), gi (__inline generate), tsa/tsl (to_screen after generate / after draw).
- LAB emp0/emp1 (&rect passed to an empty inlined static helper), e1 (corner defs through `const *screen_rect=&rect`), e3 (`register` corner vars): inert.
- ORDER EVIDENCE: January's emission order (tga*, screen_initialized, init, dispose, begin, end, is_active, to_screen, generate,
  create_noise, gen_cloud, gen_mask, draw_layer_int, screen, base, alias, set_blur, blur, enable, Smart helpers, progress_bar_draw,
  load_loading, build_glow, eachframe, setup, display) is a clean callee-before-caller source order, so generate WAS compiled before
  draw_layer_int in January -> the no-capture summary should have existed; the no-IPA lab model reproduces the signature but has
  no source-level justification found.
- No-IPA lab allocation variants xnv1 (decl order y0,x0,y1,x1), xnv2 (assign y0 before x0), xnv3 (named mask_right), xnv4 (named mask_left+right):
  all keep 640-mask in ebx and home the corners (January does the reverse); none reach January. xnv2 closest (35 differing insns vs xvt2 47).
- STORAGE (pdb_storage): January had draw_fullscreen_blur, set_blur_texture_coordinates, draw_gravy_screen_layer,
  progress_bar_screen_initialized and progress_bar_rendering_enabled FILE-STATIC (absent from cachebeta publics); ours are external.
  Probe st1 (static on blur, set_blur, rendering_enabled + draw_gravy_screen_layer): all code stays EXACT except
  draw_gravy_screen_layer becomes UNWRITTEN (unreferenced static not emitted -> January had an in-TU user of it, currently absent).
  The 3 others are code-inert storage fixes but object_audit (split-based storage=2) would flag them; PDB truth says static.
## CONCLUSION (stop rule fired)
- Verdict: NO_PROGRESS on _draw_gravy_layer (still 336 vs 352, 22/22 relocs). Real-TU probes: s1, g1, t1, t2, u1_full, st1; ~45 lab variants.
- Mechanism established: the 16-byte gap = corner values held in NON-rematerialisable variables (scratch load -> carrier copy -> spill home,
  x0 reloaded from home [ebp-4] at vertex 4). In VC7 13.00.9254 this reproduces only when generate_gravy_rect's no-capture summary is
  unavailable (lab: extern generate / generate passing rect to an unknown function) AND the corner variables are assigned at first use.
  Every tested natural spelling with the summary present collapses to production bytes. January's clean callee-before-caller emission
  order says generate WAS compiled before draw_layer_int, so the missing summary has no source explanation yet; the no-summary lab
  also misallocates (640-mask in ebx, corners homed; January the reverse).
- Optional zero-credit authenticity patch: optional_gen_mask_coord.patch (g1) - gives the orphan public gen_mask_coord its January
  caller; draw_gravy_layer/gen_mask_coord/progress_bar_draw/base_layer bytes identical to production; gate 53/1/0.
- Reopen: (a) a source construct (not a pragma/filler) that removes generate_gravy_rect's no-capture summary for draw_layer_int while
  keeping generate/to_screen exact, or (b) a VC7 remat-blocking lever for a local copied from an address-taken, non-escaping struct,
  or (c) C2 allocator replay (scratch/res9/alloc-oracle in lane-a worktree) showing which IL gives corners priority over 640-mask.
