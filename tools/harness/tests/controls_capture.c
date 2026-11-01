#include "harness.h"
#include <wchar.h>
#include <strings.h>
#define NUMBEROF(a) ((int)(sizeof(a)/sizeof((a)[0])))
#define _stricmp strcasecmp
/* sdl_platform.c's lock and clock */
typedef unsigned long long Uint64;
typedef int BOOL;
#define pthread_mutex_lock(lock) ((void)0)
#define pthread_mutex_unlock(lock) ((void)0)
static Uint64 SDL_GetTicks(void){return 0;}
struct widget_instance {
 char const *name; struct widget_instance *parent;
 union {struct {short selected_index;} list;} parameters;
};
static struct widget_instance screen,spinner,row;
static struct widget_instance *named(struct widget_instance *w,char const *name,long nth)
{(void)w;(void)nth;return !strcmp(name,"group_spinner")?&spinner:NULL;}
static struct widget_instance *screen_of(struct widget_instance *w){(void)w;return &screen;}
/* config.toml: the saved bindings, and what OK writes */
static char written[64][128];static int writes;
static char const *config_string(char const *name){(void)name;return "W, Up";}
static int config_default(char const *name,char *text,unsigned size){(void)name;snprintf(text,size,"S");return 1;}
static int config_write(char const *name,char const *value){(void)name;snprintf(written[writes++%64],128,"%s=%s",name,value);return 1;}
#define platform_log(...) ((void)0)
static void halo_input_name(int input,char *name,unsigned size){snprintf(name,size,"Key %d",input);}
static int went_back;
static void ui_widget_port_go_back(struct widget_instance *w){(void)w;went_back++;}
#include "under_test.inc"
/* sdl_platform.c's key event while a binding is waited for */
static void key_down(int scancode,int result)
{
 if(binding_capture==_binding_capture_waiting){binding_capture=_binding_capture_taken;binding_capture_result=result;binding_captured_input=scancode;}
}
static short control_named(char const *setting)
{for(short c=0;c<NUMBEROF(controls);c++)if(!strcmp(controls[c].setting,setting))return c;return NONE;}
/* A on the group's first row */
static void begin(short group)
{
 spinner.parameters.list.selected_index=group;row.name="op_command_1";row.parent=&screen;
 CHECK(control_capture_begin(&row),"no capture begun");
}
static boolean b(void){boolean deleted=FALSE;CHECK(controls_back(&screen,&deleted),"B not handled");return deleted;}
int main(int argc,char **argv)
{
 const char *case_name=argc>1?argv[1]:"";
 controls_load(FALSE);
 short forward=control_named("controls.move_forward"),fire=control_named("controls.fire");
 CASE("cancel") {
  begin(1);CHECK(controls_screen.capturing_control==fire,"not capturing FIRE");
  CHECK(!b(),"B deleted the screen while capturing");
  CHECK(!went_back,"B left the screen while capturing");
  CHECK(controls_screen.capturing_control==NONE,"capture still pending");
  CHECK(binding_capture==_binding_capture_idle,"the platform still waits for a key");
  key_down(4,1);control_capture_poll();
  CHECK(!strcmp(controls_screen.bindings[fire][0],"W")&&!strcmp(controls_screen.bindings[fire][1],"Up"),
   "FIRE changed: %s, %s",controls_screen.bindings[fire][0],controls_screen.bindings[fire][1]);
 } else CASE("leave") {
  CHECK(b(),"B kept the screen");CHECK(went_back==1,"B went back %d times",went_back);
 } else CASE("escape") {
  begin(1);key_down(41,3);control_capture_poll();
  CHECK(controls_screen.capturing_control==NONE,"Escape left the capture pending");
  CHECK(!strcmp(controls_screen.bindings[fire][0],"W"),"Escape changed FIRE");
  CHECK(b()&&went_back==1,"B after Escape did not leave");
 } else CASE("earlier-kept") {
  begin(0);key_down(7,1);control_capture_poll();
  CHECK(!strcmp(controls_screen.bindings[forward][0],"Key 7"),"MOVE FORWARD not set: %s",controls_screen.bindings[forward][0]);
  begin(1);CHECK(!b()&&!went_back,"B left the screen while capturing");
  CHECK(!strcmp(controls_screen.bindings[forward][0],"Key 7"),"MOVE FORWARD lost: %s",controls_screen.bindings[forward][0]);
  CHECK(controls_save(),"OK failed");
  CHECK(writes==1&&!strcmp(written[0],"controls.move_forward=Key 7, Up"),"OK wrote %d: %s",writes,written[0]);
 } else return 2;
 return 0;
}
