# source/rasterizer/xbox/rasterizer_xbox_shadows

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "__rasterizer_environment_shadow_model_draw"
]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/patches/rasterizer_xbox_shadows.c.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/patches/symbols.json.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/patches/config.json.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/patches/parked.json.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/final/rasterizer_xbox_shadows.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/final/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/config/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/final/config.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/final/parked.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/mk_config.py",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/LEDGER.md"
]

## production_changes
Apply scratch/w/rasterizer_xbox_shadows/production.patch. It is the concatenation of patches/{rasterizer_xbox_shadows.c,symbols.json,config.json,parked.json}.patch. `git apply --check` is clean at 931ed8dc, and the post-images are byte-identical to final/* when applied to a scratch copy.

(1) source/rasterizer/xbox/rasterizer_xbox_shadows.c:
  (a) In _rasterizer_environment_shadow_model_draw, the function-top `real_vector4d vertex_constants[3];` and its 12 stores become a bare block opened after the `model = shader_get_and_verify_type(...)` block. The block holds the aggregate initializer `real_vector4d vertex_constants[3] = {{model->detail_map_scale, model->detail_map_scale * model->detail_map_v_scale, 1.0f, 1.0f}, {1,0,0,0}, {0,1,0,0}};`, followed by shader_texture_animation_evaluate and IDirect3DDevice8_SetVertexShaderConstant(..., NUMBEROF(vertex_constants)).
  (b) Delete the invented `struct rasterizer_environment_shadows_globals`, its offsetof typedef and the `#define local_parameters rasterizer_environment_shadows_globals.local_parameters` hack. Replace them with January's '= 0' file statics in .bss offset order: local_shadow_color (real_rgb_color), local_object_bounding_radius (real), local_shadow_matrix (real_matrix4x3), local_parameters (const model-begin parameters *), then the existing shadow_setup and shadow_used. Uses are renamed accordingly. Lines that were wrapped only because of the long struct prefix are reflowed (byte-inert). The header symbol comment lists the six statics.

(2) config/symbols.json, edited in place (never re-serialised). A full copy is at scratch/w/rasterizer_xbox_shadows/config/symbols.json (= final/symbols.json).
  - Add `"static": true` to 9 XDK wrapper rows: file_offsets 1448272, 1448704, 1448784, 1449344, 1449440, 1449632, 1449664, 1449680, 1449696.
  - Replace the row `4612748 _rasterizer_environment_shadows_globals static` with 4 static rows: 4612748 `_local_shadow_color`, 4612760 `_local_object_bounding_radius`, 4612764 `_local_shadow_matrix`, 4612816 `_local_parameters`.
  - Regenerate the split (csplit). In a scratch csplit only rasterizer_xbox_shadows.obj changes, out of 833 objects.

(3) config/config.json: rasterizer_xbox_shadows.c status NonMatching -> Matching.

(4) config/parked.json: retire the park for __rasterizer_environment_shadow_model_draw (unit source/rasterizer/xbox/rasterizer_xbox_shadows).

No header edits and no other units touched.

## evidence
Baseline: gate 17/1. The only residual was model_draw, with 200/200 instructions and two transpositions: January `mov eax,[esi+0xd8]; fld [esi+0xec]; fmul [esi+0xd8]; mov [ebp-0x30],eax` and `fstp [ebp-0x2c]` before `lea edx`; ours stores first.

Mechanism:
- Lab probe lab_param_*: making `model` a copy of the parameter instead of the shader_get_and_verify_type result reproduces January's order in both plain and helper bodies. So the cause is the alias class of the call-result pointer versus the escaped vertex_constants.
- s_b1 (array declared at the top of the if-block, before the call): residual. s_b3 (array in a block opened after the call): EXACT.
- Operand order and stores-versus-initializer are inert (four variants, all EXACT).
- Control ctl_flat_topdecl: the flat statics with the old body stay residual.
- Precedent: the landed exact active_camouflage.c:610 declares the same texture-animation constants in a bare block with an aggregate initializer. C89 needs the block because the initializer reads `model`.
- /Od 0x80eba0 attests the 48-byte aggregate (RTC vsh_constants__texscale) and the [0]=scale, [1]=scale*v_scale order.
- Negatives recorded in LEDGER: unused locals 1..8 and extern decls 1..16 (oracle, inert); a lab set_real_vector4d inline (= w2 m1, 1 transposition); named real locals (x87 routing).

Object audit of the function-only candidate failed on two points:
- .bss align 4 (January) vs 8 (ours): config/contribs.json has a 74-byte contribution with flags 0xc0300080, and a >=64-byte struct forces 8.
- 9 wrapper storage rows: split 2, ours 3, absent from cachebeta publics.

HCEX compiland rasterizer_dx9_shadows.obj names the file statics local_shadow_color @+0, local_parameters @+0x44, shadow_setup +0x48 and shadow_used +0x49. The 0x38-byte gap fits radius + matrix.

Final results against the scratch split:
- lab_gate 18/18 EXACT (guard _point_from_line3d).
- audit2 PASS: 28/28 sections including .bss 74 align 4; 33/33 symbols.
- pdb_storage 0/33 disagreements.
- surplus_check: 15 surplus rows all identical to January's definers (_dot_product3d, 3 XDK tables, 6 literals, 5 __real).
- provider_link PASS.
- objdiff 3.3.1: code 5316/5316 (18/18) versus production 4633/5316, i.e. +683 meaningful / +688 padded; data 403/403.
- /W3: no new warnings. fake_match_scan: 0 leads.
- Strip S1 (zero initialisers removed) gives audit FAIL(7): 76 bytes in name-hash order. The initialisers are layout-attested.

## blockers
None for the object. Items for the reviewer and owner to note, none of them an owner-gated class:
(a) `local_object_bounding_radius` and `local_shadow_matrix` are descriptive names. HCEX leaves those 0x38 bytes unnamed; the other four static names are HCEX-attested.
(b) The model_draw fix depends on the declaration scope of the vertex-constants block. It is justified by the measured call-result alias-scope mechanism, the C89 initializer placement and the camo precedent. A strip to function-top or if-top declaration is residual, which is expected for a real scope fact.

## reopen_criteria
Not applicable: the object is complete in the candidate. If the integrator's full-ninja or stable sweep disagrees, re-run lab_gate/audit2 against a regenerated split. Follow-up lead, not in scope: January rasterizer_xbox_models.obj __rasterizer_model_draw +0x12bd shows the same January shape and our same early-store defect. Test a block-scoped vertex_constants there.

## task notes
New VC7 law (measured): a pointer returned by a call may alias only the address-taken locals that are in scope at that call. A local declared in a block opened after the call cannot be aliased, so loads through the pointer may be hoisted above stores into it.

Detector: January loads from a call-result pointer interleaved above vertex-constant stores, while ours keeps statement order.

Tools in the slug dir:
- mk_config.py: symbols.json line surgery plus csplit into split/
- lab_gate.py: gate.py with --split-root
- audit2.py: object_audit with AUDIT_SPLIT_ROOT
- surplus_check.py
- pdb_storage_cand.py
- od_report.py: objdiff 3.3.1
- warn.py: /Zs /W3
- region.py: region-order summariser

HCEX compiland dump: hcex_shadows_compiland.txt. /Od readout: od_model_draw.txt.

The integrator must regenerate build/split after applying the symbols.json edits. Against the old split, the flat-static source shows reloc-identity rows for the functions that address the split statics.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/rasterizer_xbox_shadows/LEDGER.md
