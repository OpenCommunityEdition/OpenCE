# source/cseries/profile

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/05: 11 static renames to the Sept timesection vocabulary: profile_sections_update->profile_internal_step, profile_timesection_subtract->profile_timesection_inherit, profile_frame_build_string->profile_describe_frame, profile_timer_start_at/end_at->profile_timesection_begin/end, profile_frame_dump->profile_dump_frame, profile_framedump_flush->profile_dump_frame_stop, string_starts_with->string_has_prefix, profile_sections_set_active->profile_sections_activation, profile_timer_start/end->profile_timesection_begin_now/end_now. Zero credit.

## evidence
All 11 are identical to Sept. January's assert 'parent_timesection->self_msec >= child_timesection->elapsed_msec' supports the Sept vocabulary. Gate 43/1 with SAME rows; objcmp identical; objdiff unchanged.

## blockers
_compare_profile_sections is a held January-bug candidate; object_audit FAIL(1) is unchanged.

## reopen_criteria
A contradicting first-party name. Follow-up not done: rename struct profile_timer to the timesection vocabulary.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md