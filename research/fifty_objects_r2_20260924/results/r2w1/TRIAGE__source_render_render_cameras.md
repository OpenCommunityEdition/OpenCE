# source/render/render_cameras

## decision
RETIRE

## est_probability
0.02

## route
None. Two zero-credit packets exist: production.patch (house-clean, 94.3%, applies cleanly at f6d00a8c) and production_owner_gated.patch (98.7%, 3376/113, frame 0x6c). The second needs an owner ruling on the `*(long *)&camera->vertical_field_of_view` bit read. Neither is exact.

## why_new
New probe: HCEX DIA2Dump -sym render_camera_build_frustum lists only aggregate locals: z_axis, y_axis, x_axis, viewspace_plane, viewspace_normal, frustum_midpoint, frustum_vertex, clip_plane. That is the /Od RTC set minus projected_bounds. It gives no scalar or inline inventory, so reopen criterion (b), the +1 IL symbol before CP1, is not supplied.

## prior_negatives_checked
Round-1 fn2_render_cameras, about 300 compiles. 15 REAL regions remain. CP (one M8 hunk) is solved only by an unattested (u,b)=(1,7). W-core is 14 dependency-free placement hunks (`mov eax,imm` at checks 5/10/15/21, plane-pointer reloads). They never moved under u 0..31, b 0..7, a 64-combo /Od toggle matrix, counter type, casts, inventory or helper definition order. The stop rule fired. Ledgers large_closeout_evidence_pass_20260919 and frustum_natural_reconstruction_20260913 were also read.

## notes
W-core does not depend on CP, so even an owner yes on the cast leaves the function residual.