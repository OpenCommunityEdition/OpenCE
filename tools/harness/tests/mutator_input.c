#include <string.h>
#include "harness.h"
struct script_arguments { const char *text; long output; };
static const char *argument_string(struct script_arguments *s, int n) { (void)n; return s->text; }
static void output_integer(struct script_arguments *s, long n) { s->output = n; }
static int halo_input_held_named(const char *name) { return !strcmp(name, "Left Shift"); }
#include "under_test.inc"
int main(void)
{
    struct script_arguments s;
    const char *bindings[] = {"Left Shift", "F5, Left Shift", " , Left Shift , F5", "", "F5", "Shift"};
    unsigned i;
    for (i = 0; i < sizeof(bindings)/sizeof(bindings[0]); i++) {
        s.text = bindings[i]; s.output = -1;
        CHECK(script_input_held(&s) == 1 && s.output == (i < 3), "binding list %s", bindings[i]);
    }
    return 0;
}
