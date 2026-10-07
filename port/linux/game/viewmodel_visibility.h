#ifndef VIEWMODEL_VISIBILITY_H
#define VIEWMODEL_VISIBILITY_H
/* Display only: weapon updates, sounds, gameplay and world lights continue. */
int viewmodel_is_visible(void);
int viewmodel_draws_geometry(int first_person);
#endif
