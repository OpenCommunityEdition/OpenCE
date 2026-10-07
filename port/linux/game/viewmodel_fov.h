#ifndef VIEWMODEL_FOV_H
#define VIEWMODEL_FOV_H
/* Balanced, nestable scopes change only the current local render view. */
void viewmodel_projection_begin(void);
void viewmodel_projection_end(void);
#endif
