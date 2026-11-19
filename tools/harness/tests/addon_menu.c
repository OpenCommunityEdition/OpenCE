#include <stdio.h>
#include <string.h>
#include "harness.h"
#define NONE (-1)
#define _addon_file_kind_weapon 2
static int kinds[] = {2, 1, 2, 2};
static int hidden[4];
static int port_mutators_addon_kind(long i) { return i < 4 ? kinds[i] : 0; }
static int port_mutators_addon_menu_kind(long i) { return hidden[i] ? -1 : kinds[i]; }
struct addon { long port_index, kind; long *handles; struct { long root_tag; } header; };
static struct { short count; struct addon addons[4]; } addons_globals;
#include "under_test.inc"
int main(int argc, char **argv)
{
    long handles[] = {100, 101, 102, 103};
    long i, slot = 0;
    CHECK(argc == 2, "case");
    hidden[0] = !strcmp(argv[1], "hidden_first");
    hidden[2] = !strcmp(argv[1], "hidden_middle");
    addons_globals.count = 4;
    for (i = 0; i < 4; i++) {
        addons_globals.addons[i].port_index = i;
        addons_globals.addons[i].kind = kinds[i];
        addons_globals.addons[i].handles = &handles[i];
        if (kinds[i] != 2) continue;
        if (hidden[i]) {
            CHECK(addon_slot(&addons_globals.addons[i]) == NONE, "hidden utility gets no menu slot");
            continue;
        }
        CHECK(addon_of_kind(2, slot) == i, "menu row maps physical addon");
        CHECK(addon_slot(&addons_globals.addons[i]) == slot, "same slot at runtime");
        slot++;
    }
    CHECK(addons_of_kind(2) == slot && addon_of_kind(2, slot) == NONE, "bounded enumeration");
    for (i = 0; i < slot; i++)
        CHECK(addons_weapon_definition((short)i) == handles[addon_of_kind(2, i)], "variant slot selects named weapon");
    CHECK(addons_weapon_definition(NONE) == NONE, "negative slot cannot select hidden utility");
    CHECK(addons_weapon_definition((short)slot) == NONE, "invalid slot refused");
    /* Map omits the first visible weapon: later slots must NOT compact. */
    addons_globals.addons[0] = addons_globals.addons[3];
    addons_globals.count = 1;
    CHECK(addons_weapon_definition((short)(slot - 1)) == 103, "absent map addons do not shift slots");
    CHECK(addons_weapon_definition(0) == NONE, "missing earlier slot stays empty");
    return 0;
}
