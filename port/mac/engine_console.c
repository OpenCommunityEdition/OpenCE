/* Native host counterparts for the Xbox shell-only actions. */
#ifndef __CSERIES_H
typedef unsigned char boolean;
#define FALSE 0
#endif

void platform_request_quit(void);
const char *config_string(const char *name);

boolean xbox_demos_available(void)
{
	/* The native Mac build has no Xbox dashboard demo volume to enumerate. */
	return FALSE;
}

void xbox_dashboard_launch(void)
{
	/* Leaving for the dashboard means leaving this native application. */
	platform_request_quit();
}

const char *platform_custom_edition_root(void)
{
	const char *root=config_string("paths.custom_edition");
	return root?root:"";
}
