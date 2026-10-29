/* floodfection_forms.h: Floodfection's zombies drawn as Flood combat forms
(port/linux/game/floodfection_forms.c) */
#ifndef HALO_FLOODFECTION_FORMS_H
#define HALO_FLOODFECTION_FORMS_H

struct real_orientation;
struct real_matrix4x3;

/* how an object is drawn (floodfection_forms_render) */
enum
{
	_floodfection_form_render_normal = 0,
	/* as a Flood form: its model, pose and permutations in place of its own,
	and nothing it carries */
	_floodfection_form_render_form,
	/* not at all (a zombie's dead body), nor anything it carries */
	_floodfection_form_render_hidden
};

/* a new map: the forms' tags it has (copied in by level_tag_import.c), and no
zombie drawn as one yet (infection.c's initialize_for_new_map) */
void floodfection_forms_reset(void);
/* each tick, each player (infection.c, every machine): whether a
Floodfection zombie (else none of this) */
void floodfection_forms_update(long player_index, boolean zombie);
/* how the object is drawn (render_objects.c), with what in place of its own */
short floodfection_forms_render(long object_index, long *model_index, struct real_matrix4x3 const **node_matrices,
	byte const **region_permutations);
/* the target the way from the original by the fraction, into the target */
void floodfection_forms_blend(short node_count, struct real_orientation *original, struct real_orientation *target,
	real fraction);

#endif
