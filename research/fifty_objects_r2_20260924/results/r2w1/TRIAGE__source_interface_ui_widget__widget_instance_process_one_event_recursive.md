# source/interface/ui_widget:_widget_instance_process_one_event_recursive

## decision
OWNER

## est_probability
0.9

## route
Two changes: the structural-lane P3 body (/Od 0x668e50 topology) and the held dpad_event_times .bss split. The split is one symbols.json static entry at file_offset 4541104 (storage+0x870) plus a standalone `static unsigned long dpad_event_times[MAXIMUM_NUMBER_OF_LOCAL_PLAYERS][NUMBER_OF_DPAD_DIRECTIONS];`. The structural lane measured 97/5 with it (the function EXACT, no row lost).

## why_new
I re-measured at f6d00a8c: structural candidate.c plus the one include canonical has added since gives exact 96 / residual 6. alndiff shows the only remaining difference in poe is January `lea eax,[edx+ecx*4-8]` with storage:2160 against ours `lea eax,[edx+ecx*4]` with 2128. The effective address is the same, so January's array is a separate object starting at storage+0x870. The name has no first-party source: HCEX _widget_globals has none, /Od has no dpad code, cachebeta has no public, and the atlas has no ui_widget data names.

## prior_negatives_checked
ui_widget_obj_opus5_150k_w3 (split held under the invented .bss names ruling); opus5_100k; C:/halo-worktrees/claude-structural-20260923/scratch/lane/w/interface__ui_widget/REPORT.md (P1-P4, split measurement).

## notes
This is the same held class as owner-queue item 2 (rasterizer_xbox .bss split with descriptive static names). Worth +2016 B. It does NOT complete ui_widget, because five tie residuals remain.