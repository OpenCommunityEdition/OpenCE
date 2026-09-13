# flag_render_proper retained research candidate (2026-09-12)

Status: archived research only; not admitted to production source and receives zero exact credit.

The Claude candidate reconstructs the complete cloth mesh and render flow and reaches 98.77677% objdiff with the exact 2,112-byte padded envelope and all 84 relocation identities. It is not publication-safe: it introduces TU-local facades for shared rasterizer model types, an unauthenticated numeric lock constant, repeated raw object_get access, an apparently unused object_definition_get call, and expression/layout choices consistent with compiler steering. The pre-existing object ledger independently rejected this owner class pending authoritative declarations. Reopen only with original January/retail symbol or source evidence for the shared types and lock enum, or a natural same-compiler donor that accounts for the remaining instruction selection.

Target: size 2112, relocations 84, normalized SHA-256 d0507f5d0637539037a0bda0c9c0e1a1c0a0753e04913603fcdd66e3f46f6416.

Candidate: size 2112, relocations 84, normalized SHA-256 a5b00839955717b8a04a2127124eb2bed43603450324a386284f1379931100df.

Archived patch:

```diff
diff --git a/source/objects/widgets/flags.c b/source/objects/widgets/flags.c
index 01007d625..417769cb2 100644
--- a/source/objects/widgets/flags.c
+++ b/source/objects/widgets/flags.c
@@ -64,11 +64,18 @@ symbols in this file:

 #include "cseries/cseries.h"
 #include "cseries/errors.h"
+#include "game/game.h"
 #include "memory/data.h"
+#include "objects/object_definitions.h"
 #include "objects/objects.h"
 #include "physics/point_physics.h"
+#include "rasterizer/rasterizer.h"
+#include "rasterizer/rasterizer_geometry.h"
+#include "render/render.h"
 #include "saved games/game_state.h"
 #include "scenario/scenario.h"
+#include "shaders/shader_definitions.h"
+#include "shaders/shaders.h"
 #include "tag_files/tag_groups.h"

 /* ---------- constants */
@@ -92,6 +99,11 @@ enum trailing_edge_shape
	_trailing_edge_shape_trapezoid_short_bottom
 };

+enum
+{
+	_rasterizer_lock_flag = 11,
+};
+
 enum attached_edge_shape
 {
	_attached_edge_shape_flat,
@@ -119,6 +131,56 @@ enum tesselate

 /* ---------- structures */

+struct model_vertex_uncompressed
+{
+	real_point3d position;
+	real_vector3d normal;
+	real_vector3d binormal;
+	real_vector3d tangent;
+	real_point2d texcoord;
+	short nodes[2];
+	real node_weights[2];
+};
+
+struct model_vertex_compressed
+{
+	real_point3d position;
+	unsigned long normal;
+	unsigned long binormal;
+	unsigned long tangent;
+	point2d texcoord;
+	byte nodes[2];
+	short node_weight;
+};
+
+struct rasterizer_model_skinning
+{
+	real_matrix4x3 const *node_matrices;
+	short node_matrix_count;
+	word pad;
+};
+
+struct render_model_effect
+{
+	short type;
+	word pad;
+	real intensity;
+	byte reserved[0x20];
+};
+
+struct rasterizer_model_begin_parameters
+{
+	unsigned long geometry_flags;
+	long unique_identifier;
+	struct rasterizer_model_skinning skinning;
+	struct render_lighting lighting;
+	struct render_animation animation;
+	struct render_model_effect effect;
+	real_point3d centroid;
+	real radius;
+	real_vector2d base_map_scale;
+};
+
 struct flag_datum_prefix
 {
	struct datum_header header;
@@ -669,6 +731,237 @@ struct flag_cell_datum *flag_datum_get_cell(
	return &flag->cells[x * (definition->height - 1) + y];
 }

+void flag_render_proper(
+	struct flag_datum_prefix *flag,
+	struct flag_definition *definition,
+	struct render_lighting const *lighting,
+	struct render_animation const *animation)
+{
+	long shader_index;
+	short maximum_triangles;
+	long triangle_buffer_index;
+	long vertex_buffer_index;
+
+	if (object_get(flag->object_index)->object.owner_team_index != _team_red)
+		shader_index = definition->shader_blue.index;
+	else
+		shader_index = definition->shader_red.index;
+	if (shader_index == NONE)
+		shader_index = definition->shader_blue.index;
+
+	rasterizer_globals.current_lock_operation = _rasterizer_lock_flag;
+	maximum_triangles = (2*definition->height - 2)*(definition->width - 1);
+	triangle_buffer_index = rasterizer_dynamic_triangles_new(maximum_triangles);
+	vertex_buffer_index = rasterizer_dynamic_vertices_new(
+		_rasterizer_vertex_type_model_compressed,
+		definition->width*definition->height);
+	if (triangle_buffer_index != NONE && vertex_buffer_index != NONE)
+	{
+		struct model_vertex_compressed *vertices;
+		short *triangles;
+		real one_over_width;
+		real one_over_height;
+		struct model_vertex_uncompressed vertex;
+		struct rasterizer_model_begin_parameters parameters;
+		struct shader *shader;
+		struct flag_vertex_datum *corner00;
+		struct flag_vertex_datum *corner10;
+		struct flag_vertex_datum *corner01;
+		struct flag_vertex_datum *corner11;
+		short vertex_index = 0;
+		short i;
+		short x;
+		short y;
+
+		object_definition_get(object_get(flag->object_index)->definition_index);
+		vertices = rasterizer_dynamic_vertices_lock(vertex_buffer_index);
+		triangles = rasterizer_dynamic_triangles_lock(triangle_buffer_index);
+		one_over_width = 1.0f/(definition->width - 1);
+		one_over_height = 1.0f/(definition->height - 1);
+		csmemset(&vertex, 0, sizeof(vertex));
+
+		for (x = 0; x < definition->width; x++)
+		{
+			for (y = 0; y < definition->height; y++, vertex_index++)
+			{
+				struct flag_vertex_datum *point = flag_datum_get_vertex(flag, definition, x, y);
+				short x0 = x < definition->width - 1 ? x : x - 1;
+				short y0 = y < definition->height - 1 ? y : y - 1;
+				struct flag_vertex_datum *origin = flag_datum_get_vertex(flag, definition, x0, y0);
+				struct flag_vertex_datum *across = flag_datum_get_vertex(flag, definition, x0 + 1, y0);
+				struct flag_vertex_datum *down = flag_datum_get_vertex(flag, definition, x0, y0 + 1);
+				real_vector3d horizontal;
+				real_vector3d vertical;
+
+				vector_from_points3d(&origin->position, &across->position, &horizontal);
+				vector_from_points3d(&origin->position, &down->position, &vertical);
+				cross_product3d(&horizontal, &vertical, &vertex.normal);
+				normalize3d(&vertex.normal);
+
+				vertex.position = point->position;
+				vertex.texcoord.x = x*one_over_width;
+				vertex.texcoord.y = y*one_over_height;
+				rasterizer_geometry_compress_vertices(
+					_rasterizer_vertex_type_model_uncompressed,
+					1,
+					&vertices[vertex_index],
+					sizeof(struct model_vertex_compressed),
+					&vertex,
+					sizeof(struct model_vertex_uncompressed));
+			}
+		}
+
+		i = 0;
+		for (x = 0; x < definition->width - 1; x++)
+		{
+			for (y = 0; y < definition->height - 1; y++)
+			{
+				switch (flag_datum_get_cell(flag, definition, x, y)->tesselation)
+				{
+					case _tesselate_both:
+						match_assert(
+							"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
+							742,
+							i<maximum_triangles);
+						triangles[3*i + 0] = definition->height*x + y;
+						triangles[3*i + 1] = definition->height*(x + 1) + y;
+						triangles[3*i + 2] = definition->height*x + y + 1;
+						i++;
+						match_assert(
+							"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
+							747,
+							i<maximum_triangles);
+						triangles[3*i + 0] = definition->height*x + y + 1;
+						triangles[3*i + 1] = definition->height*(x + 1) + y;
+						triangles[3*i + 2] = definition->height*(x + 1) + y + 1;
+						i++;
+						break;
+
+					case _tesselate_none:
+						break;
+
+					case _tesselate_top_left:
+						match_assert(
+							"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
+							754,
+							i<maximum_triangles);
+						triangles[3*i + 0] = definition->height*x + y;
+						triangles[3*i + 1] = definition->height*x + y + 1;
+						triangles[3*i + 2] = definition->height*(x + 1) + y;
+						i++;
+						break;
+
+					case _tesselate_bottom_left:
+						match_assert(
+							"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
+							761,
+							i<maximum_triangles);
+						triangles[3*i + 0] = definition->height*x + y;
+						triangles[3*i + 1] = definition->height*x + y + 1;
+						triangles[3*i + 2] = definition->height*(x + 1) + y + 1;
+						i++;
+						break;
+
+					case _tesselate_top_right:
+						match_assert(
+							"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
+							768,
+							i<maximum_triangles);
+						triangles[3*i + 0] = definition->height*x + y;
+						triangles[3*i + 1] = definition->height*(x + 1) + y + 1;
+						triangles[3*i + 2] = definition->height*(x + 1) + y;
+						i++;
+						break;
+
+					case _tesselate_bottom_right:
+						match_assert(
+							"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
+							775,
+							i<maximum_triangles);
+						triangles[3*i + 0] = definition->height*x + y + 1;
+						triangles[3*i + 1] = definition->height*(x + 1) + y + 1;
+						triangles[3*i + 2] = definition->height*(x + 1) + y;
+						i++;
+						break;
+
+					default:
+						match_vassert(
+							"c:\\halo\\SOURCE\\objects\\widgets\\flags.c",
+							784,
+							FALSE,
+							NULL);
+						break;
+				}
+			}
+		}
+
+		rasterizer_dynamic_triangles_unlock(triangle_buffer_index);
+		rasterizer_dynamic_vertices_unlock(vertex_buffer_index);
+		shader = shader_definition_get(shader_index);
+
+		{
+			real_point3d centroid;
+
+			corner00 = flag_datum_get_vertex(flag, definition, 0, 0);
+			corner10 = flag_datum_get_vertex(flag, definition, definition->width - 1, 0);
+			corner01 = flag_datum_get_vertex(flag, definition, 0, definition->height - 1);
+			corner11 = flag_datum_get_vertex(flag, definition, definition->width - 1, definition->height - 1);
+			centroid.x = (corner11->position.x + corner01->position.x + corner10->position.x +
+				corner00->position.x)*0.25f;
+			centroid.y = (corner11->position.y + corner01->position.y + corner10->position.y +
+				corner00->position.y)*0.25f;
+			centroid.z = (corner11->position.z + corner01->position.z + corner10->position.z +
+				corner00->position.z)*0.25f;
+
+			csmemset(&parameters, 0, sizeof(parameters));
+			parameters.unique_identifier = 1;
+			parameters.skinning.node_matrices = global_identity4x3;
+			parameters.skinning.node_matrix_count = 1;
+			parameters.lighting = *lighting;
+			parameters.animation = *animation;
+			parameters.centroid = centroid;
+			parameters.base_map_scale.i = parameters.base_map_scale.j = 1.0f;
+
+			rasterizer_profile_enable(FALSE);
+			rasterizer_models_begin(FALSE);
+			rasterizer_model_begin(&parameters, FALSE);
+			if (shader_type_is_transparent(shader->base.type))
+			{
+				rasterizer_model_transparent_geometry_submit(
+					shader,
+					0,
+					NULL,
+					triangle_buffer_index,
+					i,
+					NULL,
+					vertex_buffer_index,
+					&centroid,
+					NULL);
+			}
+			else
+			{
+				rasterizer_model_draw(
+					shader,
+					0,
+					NULL,
+					triangle_buffer_index,
+					i,
+					NULL,
+					vertex_buffer_index);
+			}
+			rasterizer_model_end();
+			rasterizer_models_end();
+			rasterizer_profile_enable(TRUE);
+		}
+	}
+
+	rasterizer_dynamic_triangles_delete(triangle_buffer_index);
+	rasterizer_dynamic_vertices_delete(vertex_buffer_index);
+	rasterizer_globals.current_lock_operation = _rasterizer_lock_unlocked;
+
+	return;
+}
+
 void flag_tesselate_region(
	struct flag_definition *definition,
	struct flag_datum_prefix *flag,

```
