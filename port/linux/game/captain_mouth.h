/*
CAPTAIN_MOUTH.H

Captain Keyes's lower lip, level where no emotion's frame levels it
(captain_mouth.c).
*/

#ifndef __CAPTAIN_MOUTH_H
#define __CAPTAIN_MOUTH_H

struct real_orientation;

/* an Xbox map's tags have loaded: the lip is measured (cache_files.c) */
void captain_mouth_tags_loaded(void);

/* a unit's pose, once its emotion's frame has been laid over it
(emotion_applied) or it has none: the captain's, with no emotion, has his
lower lip levelled (units.c, unit_preprocess_node_orientations) */
void captain_mouth_node_orientations(long unit_index, boolean emotion_applied, struct real_orientation *node_orientations);

#endif
