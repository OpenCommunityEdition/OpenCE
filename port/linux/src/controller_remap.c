/* Controller-owned configuration. Empty/invalid layouts retain the profile.
 * Stored names describe physical positions; artwork preferences never affect input. */
#include "controller_remap.h"
#include "controller_buttons.h"
#include "port_config.h"
#include <stdio.h>
#include <string.h>

unsigned char const controller_remap_controls[CONTROLLER_REMAP_ACTIONS] = {0,1,2,3,4,5,6,7,10,11};
static unsigned char const remap_buttons[CONTROLLER_REMAP_BUTTONS] = {0,1,2,3,4,5,6,7,14,15,10};
static char const *const remap_names[] = {"south","east","west","north","rb","lb","lt","rt","ls","rs","dpad_left"};

char const *controller_remap_setting(short port)
{
    static char const *const settings[] = {"input.controller_p1", "input.controller_p2",
        "input.controller_p3", "input.controller_p4"};
    return port >= 0 && port < 4 ? settings[port] : NULL;
}

int controller_remap_parse(char const *text, unsigned char *buttons)
{
    unsigned char parsed[CONTROLLER_REMAP_ACTIONS];
    unsigned int used = 0;
    int action, i;
    if (!text || !*text) return 0;
    for (action = 0; action < CONTROLLER_REMAP_ACTIONS; action++)
    {
        size_t length = strcspn(text, ",");
        for (i = 0; i < CONTROLLER_REMAP_BUTTONS; i++)
            if (strlen(remap_names[i]) == length && !strncmp(text, remap_names[i], length)) break;
        if (i == CONTROLLER_REMAP_BUTTONS || (used & (1u << i))) return 0;
        used |= 1u << i;
        parsed[action] = remap_buttons[i];
        text += length;
        if (action < CONTROLLER_REMAP_ACTIONS - 1)
        {
            if (*text++ != ',') return 0;
        }
        else if (*text) return 0;
    }
    memcpy(buttons, parsed, sizeof(parsed));
    return 1;
}

void controller_remap_format(unsigned char const *buttons, char *text, unsigned int size)
{
    unsigned int used = 0;
    int action, i;
    if (!size) return;
    *text = 0;
    for (action = 0; action < CONTROLLER_REMAP_ACTIONS; action++)
    {
        for (i = 0; i < CONTROLLER_REMAP_BUTTONS && remap_buttons[i] != buttons[action]; i++);
        if (i == CONTROLLER_REMAP_BUTTONS) { *text = 0; return; }
        if (used + strlen(remap_names[i]) + 2 > size) { *text = 0; return; }
        used += (unsigned int)snprintf(text + used, size - used, "%s%s", action ? "," : "", remap_names[i]);
    }
}

int controller_remap_read(short port, unsigned char *buttons)
{
    static unsigned long read_at = (unsigned long)-1;
    static unsigned char layouts[4][CONTROLLER_REMAP_ACTIONS];
    static int valid[4];
    int i;
    if (port < 0 || port >= 4) return 0;
    if (read_at != config_changes())
    {
        for (i = 0; i < 4; i++)
            valid[i] = controller_remap_parse(config_string(controller_remap_setting((short)i)), layouts[i]);
        read_at = config_changes();
    }
    if (valid[port]) memcpy(buttons, layouts[port], CONTROLLER_REMAP_ACTIONS);
    return valid[port];
}

void controller_remap_swap(unsigned char *buttons, int action, int button)
{
    int i;
    if (action < 0 || action >= CONTROLLER_REMAP_ACTIONS) return;
    for (i = 0; i < CONTROLLER_REMAP_BUTTONS && remap_buttons[i] != button; i++);
    if (i == CONTROLLER_REMAP_BUTTONS) return;
    for (i = 0; i < CONTROLLER_REMAP_ACTIONS; i++)
        if (buttons[i] == button)
        {
            buttons[i] = buttons[action];
            break;
        }
    buttons[action] = (unsigned char)button;
}

int controller_remap_cycle(int button, int direction)
{
    int i;
    for (i = 0; i < CONTROLLER_REMAP_BUTTONS; i++)
        if (remap_buttons[i] == button) return remap_buttons[(i + (direction < 0 ? CONTROLLER_REMAP_BUTTONS - 1 : 1)) % CONTROLLER_REMAP_BUTTONS];
    return 0;
}

char const *controller_remap_button_name(int button, int family)
{
    static char const *const labels[4][CONTROLLER_REMAP_BUTTONS] = {
        {"A","B","X","Y","RB / BLACK","LB / WHITE","LT","RT","LEFT STICK","RIGHT STICK","D-PAD LEFT"},
        {"A","B","X","Y","RB / BLACK","LB / WHITE","LT","RT","LEFT STICK","RIGHT STICK","D-PAD LEFT"},
        {"CROSS","CIRCLE","SQUARE","TRIANGLE","R1","L1","L2","R2","L3","R3","D-PAD LEFT"},
        {"B","A","Y","X","R","L","ZL","ZR","LEFT STICK","RIGHT STICK","D-PAD LEFT"}
    };
    int i;
    if (family < 0 || family > 3) family = 0;
    for (i = 0; i < CONTROLLER_REMAP_BUTTONS; i++)
        if (remap_buttons[i] == button) return labels[family][i];
    return "-";
}
