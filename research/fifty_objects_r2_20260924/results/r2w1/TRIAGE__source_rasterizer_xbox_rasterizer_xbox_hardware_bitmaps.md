# source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps

## decision
RETIRE

## est_probability
0.03

## route
None for _rasterizer_bitmap_new. The residual is one jne displacement at +0x15A: January binds it to the epilogue after the error() block (0x171), we bind it to the byte-identical clone after the no-device block (0x17e). Keep the park. Zero-credit hygiene the integrator may take whenever the object is next touched: rename 0x158120 _rasterizer_bitmap_cube_map_changed -> _rasterizer_bitmap_cm_changed (2001-09-25 cachebeta-xbe map 0x166dd0, anchored with delta 0xECB0); remove the consumer-local prototypes texture_cache_bitmap_delete (l.101) and rasterizer_error (l.103) and the consumer-local `extern D3DDevice *global_d3d_device` (l.116) in favour of the owner headers.

## why_new
Nothing new. The two returns (the no-device return TRUE, emitted as mov al,bl from the ebx=1 pin, and return success) merge on VALUE, and the canonical block follows the layout-last fall-through predecessor. Round-1 reopen criterion (a) needs a return that is IL-distinct but byte-identical. The candidates I checked (return success on the ND path, return !global_d3d_device, hw==NULL) either constant-fold to TRUE and merge again, or emit different text. There is no new first-party source: /Od 0x7ea7d0 is the dx9 rewrite, and HCEX is a PPC rewrite with no locals.

## prior_negatives_checked
Park evidence (>50 control-flow shapes). opus5 crossjump-epilogue R3-R8 (38 whole-TU spellings, 24 case orders, declaration count 1..32). Lane C block-selection FINDINGS (~30 probes incl. A3/A4 single exit, er/els early guard, m1). Round-1 wave2 a1/a2 (/Od arms regress to 432 B), a4, t7/t7b (/FAsc: ND sunk but the canonical still follows ND), s1. The 33-instance FIRST-binding corpus study. The block-selection-is-source-reachable and tail-merge laws (value-specialised return text is the only known lever, and it does not apply here).

## notes
Stop rule has fired twice (the park and round 1). The object has no other blocker besides this function and the hygiene above. The XDK header tables (D3DPRIMITIVETOVERTEXCOUNT etc.) would need the surplus_identity/provider_link audit if the function ever closed.