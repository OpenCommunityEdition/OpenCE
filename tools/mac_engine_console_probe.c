#include <stdio.h>
#include <string.h>
typedef unsigned char boolean;
#define FALSE 0

static int quit_requested;
static const char *configured_root="/games/halo-ce";
void platform_request_quit(void) { quit_requested++; }
const char *config_string(const char *name)
{ return name&&strcmp(name,"paths.custom_edition")==0?configured_root:NULL; }
boolean xbox_demos_available(void);
void xbox_dashboard_launch(void);
const char *platform_custom_edition_root(void);

int main(void)
{
	int failures=0;
	if(xbox_demos_available()!=FALSE)failures++;
	xbox_dashboard_launch();
	if(quit_requested!=1)failures++;
	if(strcmp(platform_custom_edition_root(),configured_root)!=0)failures++;
	configured_root=NULL;
	if(strcmp(platform_custom_edition_root(),"")!=0)failures++;
	printf("mac native shell boundary: 4 checks, %d failures\n",failures);
	return failures?1:0;
}
