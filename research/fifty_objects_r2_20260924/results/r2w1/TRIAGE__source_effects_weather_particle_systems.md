# source/effects/weather_particle_systems

## decision
OWNER

## est_probability
0.25

## route
(1) Owner ruling on Lane D class I (HANDOFF_20260922 l.142): the /Od-attested dead scalar `short unused_count = 0;`, from init-only slot [ebp-0x24] at /Od 0x57fbb4, with an invented name. On yes, apply round-1 production.patch. (2) Provider dependency: the _real_local_random surplus conflicts with effects.obj's NODUP named-result copy. The fix is a real_math.h body change (`real result = real_seed_random(...); return result;`) plus removing the effects.c hand copy. This is inside the Codex real_math reservation. It needs a full-board re-sweep, because tree D (0914) broke units before Layer 2 changed the name-count landscape. The separable zero-credit fix, dropping `static` from one_over_char_max (cachebeta public 0x25AAFC), can land now.

## why_new
Measured at f6d00a8c. Round-1 fin2.c re-gates 20/20 EXACT with object_audit PASS, but provider_link still FAILs(1) on _real_local_random. New census: January's split defines _real_local_random ONLY in effects.obj, and only first_person_weapons references it. Our base emits the same COMDAT in effects, weather, breakable_surfaces and rasterizer_xbox_environment_fog. So weather's copy is folded surplus from the header inline, whose `return real_seed_random(...)` body ends `pop ecx` while January's selected copy ends `add esp,4`.

## prior_negatives_checked
Round-1 fn_weather and REVIEW: the strip test fails without unused_count, and the review defers to the owner. opus5_150k_w1: tree D (named-result real_local_random) gave no gain and a units loss. fresh_graphs and helper_recovery_20260923 were read. Render shapes to not repeat: box_count=1, box-offset positions, point-first transform dot.

## notes
Completion needs BOTH the class-I yes AND the provider repair, which depends on real_math.h (Codex-reserved) and a zero-regression sweep. Once the reservation is released, the tree-D header body is worth re-measuring at the current base.