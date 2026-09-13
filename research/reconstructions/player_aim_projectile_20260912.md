# player_aim_projectile retained research candidate (2026-09-12)

Status: archived research only; not admitted to production source and receives zero exact credit.

The Claude candidate reconstructs the complete aim-magnetism and projectile trace flow and reaches 99.24812% objdiff with the exact 832-byte padded envelope and all 42 relocation identities. It is not publication-safe in its present form: the source has unsupported block nesting, point/vector representation casts used for in-place point addition, and a newly exposed helper owner whose January inline/out-of-line schedule has not been authenticated. The semantic candidate is preserved here without granting matching credit. Reopen only with authoritative source/symbol context for player_aim_projectile and pin_normal_to_cone3d, or a natural same-compiler donor that explains the point update and helper schedule.

Target: size 832, relocations 42, normalized SHA-256 79f83e1a1b6ec5a6a83191382cd17fdb82c6da8f216f050b4c4379a4be525ea7.

Candidate: size 832, relocations 42, normalized SHA-256 68607485f783d5000d94b7e70c70f1b33142754bdb5e67c904c3a128d47e5f02.

Archived patch (the unrelated REAL_MATH_EXTERNAL_REAL_RANDOM_RANGE hunk belongs to another lane and must not be interpreted as part of this candidate):

```diff
diff --git a/source/game/aim_assist.c b/source/game/aim_assist.c
index b5e833a93..cda42c6d7 100644
--- a/source/game/aim_assist.c
+++ b/source/game/aim_assist.c
@@ -547,6 +547,118 @@ boolean aim_assist(
	return FALSE;
 }

+long player_aim_projectile(
+	long player_index,
+	real_point3d const *position,
+	real_vector3d *direction)
+{
+	long hit_object_index = NONE;
+	struct player_datum *player = player_get(player_index);
+	long aiming_unit_index = unit_get_aiming_unit_index(player->unit_index);
+
+	match_assert_valid_real_normal3d(
+		"c:\\halo\\SOURCE\\game\\aim_assist.c",
+		77,
+		direction);
+	match_assert(
+		"c:\\halo\\SOURCE\\game\\aim_assist.c",
+		79,
+		global_current_collision_user_depth < MAXIMUM_COLLISION_USER_STACK_DEPTH);
+	global_current_collision_users[global_current_collision_user_depth++] = _collision_user_aim_assist;
+
+	{
+		short zoom_level = unit_get_zoom_level(aiming_unit_index);
+		struct aim_assist_parameters parameters;
+
+		if (unit_get_aim_assist_parameters(aiming_unit_index, zoom_level, &parameters))
+		{
+			real_point3d camera_position;
+			real_vector3d camera_forward;
+
+			director_camera_deterministic(player->unit_index, &camera_position, &camera_forward);
+
+			{
+				real_vector3d aim_direction = *direction;
+				real magnetism_level = 0.f;
+
+				{
+					struct aim_assist_target target;
+
+					if (aim_assist(
+						&parameters,
+						&camera_position,
+						&camera_forward,
+						player->unit_index,
+						(short)player->team_index,
+						&target))
+					{
+						vector_from_points3d(position, &target.position, &aim_direction);
+						if (normalize3d(&aim_direction) == 0.f)
+						{
+							aim_direction = *direction;
+						}
+
+						magnetism_level = target.autoaim_level;
+						hit_object_index = target.object_index;
+					}
+				}
+
+			{
+					struct unit_datum *aiming_unit = unit_get(aiming_unit_index);
+					real camera_to_unit_distance = distance3d(&aiming_unit->object.position, &camera_position);
+					real_vector3d camera_direction = camera_forward;
+					real_vector3d vector;
+					struct collision_result collision;
+					real_vector3d impact_direction;
+
+					normalize3d(&camera_direction);
+					scale_vector3d(&camera_direction, camera_to_unit_distance, &camera_direction);
+					add_vectors3d(
+						&camera_direction,
+						(real_vector3d const *)&camera_position,
+						(real_vector3d *)&camera_position);
+					scale_vector3d(&camera_forward, 128.f, &vector);
+					collision_test_vector(
+						_collision_test_for_projectiles_flags,
+						&camera_position,
+						&vector,
+						player->unit_index,
+						&collision);
+
+					vector_from_points3d(position, &collision.point, &impact_direction);
+					{
+						real impact_magnitude = normalize3d(&impact_direction);
+
+						if (impact_magnitude == 0.f)
+						{
+							impact_direction = *direction;
+						}
+					}
+
+					fast_normals_interpolate(&impact_direction, &aim_direction, magnetism_level, &vector);
+					pin_normal_to_cone3d(
+						&vector,
+						direction,
+						sine(parameters.deviation_angle),
+						cosine(parameters.deviation_angle),
+						direction);
+				}
+			}
+		}
+	}
+
+	match_assert(
+		"c:\\halo\\SOURCE\\game\\aim_assist.c",
+		140,
+		global_current_collision_user_depth > 1);
+	--global_current_collision_user_depth;
+
+	player->aim_assist_unit_index = hit_object_index;
+	player->aim_assist_timestamp = game_time_get();
+
+	return hit_object_index;
+}
+
 long local_player_aim_assist(
	short local_player_index,
	real *autoaim_level,
diff --git a/source/game/aim_assist.h b/source/game/aim_assist.h
index 0981f4d52..f5570893c 100644
--- a/source/game/aim_assist.h
+++ b/source/game/aim_assist.h
@@ -59,6 +59,10 @@ boolean aim_assist(
	long ignore_object_index,
	short ignore_team_index,
	struct aim_assist_target *target);
+long player_aim_projectile(
+	long player_index,
+	real_point3d const *position,
+	real_vector3d *direction);
 long local_player_aim_assist(
	short local_player_index,
	real *autoaim_level,
diff --git a/source/math/real_math.h b/source/math/real_math.h
index 68f7673fc..3f467ccce 100644
--- a/source/math/real_math.h
+++ b/source/math/real_math.h
@@ -560,6 +560,12 @@ void pitch_vectors(real_vector3d *forward, real_vector3d *up, real sine, real co

 void fast_normals_interpolate(real_vector3d const *a, real_vector3d const *b, real t, real_vector3d *result);
 void normals_interpolate(real_vector3d const *a, real_vector3d const *b, real t, real_vector3d *result);
+boolean pin_normal_to_cone3d(
+	real_vector3d const *normal,
+	real_vector3d const *direction,
+	real sine,
+	real cosine,
+	real_vector3d *result);
 real_vector3d *reflect_vector3d(real_vector3d const *incident, real_vector3d const *normal, real_vector3d *reflection);
 real_vector3d *refract_vector3d(real_vector3d const *incident, real_vector3d const *normal, real coefficient_of_refraction, real_vector3d *refraction);

@@ -1446,12 +1452,18 @@ __inline real real_random(
	return real_seed_random(get_global_random_seed_address());
 }

+#ifdef REAL_MATH_EXTERNAL_REAL_RANDOM_RANGE
+real real_random_range(
+	real lower_bound,
+	real upper_bound);
+#else
 __inline real real_random_range(
	real lower_bound,
	real upper_bound)
 {
	return real_seed_random_range(get_global_random_seed_address(), lower_bound, upper_bound);
 }
+#endif

 __inline real_vector3d *random_direction3d(
	real_vector3d *direction)

```
