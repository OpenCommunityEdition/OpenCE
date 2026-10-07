#include "viewmodel_visibility.h"

int config_boolean(const char *name);
unsigned long config_changes(void);

int viewmodel_is_visible(void)
{
	static int initialized, visible;
	static unsigned long read_at;
	unsigned long changes = config_changes();
	if (!initialized || read_at != changes)
	{
		visible = config_boolean("display.viewmodel_visible");
		initialized = 1;
		read_at = changes;
	}
	return visible;
}

int viewmodel_draws_geometry(int first_person)
{
	return !first_person || viewmodel_is_visible();
}
