/* The controller editor follows Controls Setup's pending Save/Cancel/Defaults
 * convention, with one complete layout per input port and no profile mutation. */
static wchar_t const *const pad_action_labels[CONTROLLER_REMAP_ACTIONS] = {
    L"JUMP", L"SWITCH GRENADE", L"USE / RELOAD", L"SWITCH WEAPON", L"MELEE",
    L"FLASHLIGHT", L"THROW GRENADE", L"FIRE", L"CROUCH", L"ZOOM"
};
static short const pad_action_groups[CONTROLLER_REMAP_ACTIONS] = {0,1,0,1,1,0,1,1,0,1};
static struct
{
    unsigned char buttons[4][CONTROLLER_REMAP_ACTIONS];
    boolean custom[4];
    short capturing, port;
} pad_editor;

static short pad_editor_port(struct widget_instance *screen)
{
    struct widget_instance *spinner = named(screen, "pad_player", 0);
    short port = spinner ? spinner->parameters.list.selected_index : 0;
    return port >= 0 && port < 4 ? port : 0;
}

static short pad_editor_action(struct widget_instance *widget)
{
    struct widget_instance *group = named(screen_of(widget), "pad_group", 0);
    short wanted, action;
    for (; widget && strncmp(widget->name, "op_pad_", 7); widget = widget->parent);
    if (!widget) return NONE;
    wanted = (short)(atoi(widget->name + 7) - 1);
    for (action = 0; action < CONTROLLER_REMAP_ACTIONS && wanted >= 0; action++)
        if (pad_action_groups[action] == (group ? group->parameters.list.selected_index : 0) && !wanted--)
            return action;
    return NONE;
}

static boolean pad_editor_event(struct widget_instance *widget, char const *name)
{
    struct widget_instance *screen = screen_of(widget);
    short port = pad_editor_port(screen), action, i;
    if (!strcmp(name, "port controller init"))
    {
        struct widget_instance *spinner = named(screen, "pad_player", 0);
        controller_remap_capture_end();
        pad_editor.capturing = NONE;
        for (i = 0; i < 4; i++)
        {
            controller_remap_profile(i, pad_editor.buttons[i]);
            pad_editor.custom[i] = (boolean)controller_remap_read(i, pad_editor.buttons[i]);
        }
        if (spinner) spinner->parameters.list.selected_index = controller_of(widget);
    }
    else if (!strcmp(name, "port controller close"))
    {
        pad_editor.capturing = NONE;
        controller_remap_capture_end();
    }
    else if (!strcmp(name, "port controller defaults"))
    {
        controller_remap_profile(port, pad_editor.buttons[port]);
        pad_editor.custom[port] = FALSE;
    }
    else if (!strcmp(name, "port controller save"))
    {
        boolean result = TRUE;
        for (i = 0; i < 4; i++)
        {
            char text[CONTROLLER_REMAP_TEXT] = {0};
            if (pad_editor.custom[i]) controller_remap_format(pad_editor.buttons[i], text, sizeof(text));
            if (strcmp(text, config_string(controller_remap_setting(i))) &&
                !config_write(controller_remap_setting(i), text)) result = FALSE;
        }
        return result;
    }
    else if ((action = pad_editor_action(widget)) != NONE)
    {
        if (!strcmp(name, "port controller bind"))
        {
            pad_editor.capturing = action;
            pad_editor.port = port;
            controller_remap_capture_begin(port);
        }
        else
        {
            int direction = !strcmp(name, "port controller left") ? -1 : 1;
            controller_remap_swap(pad_editor.buttons[port], action,
                controller_remap_cycle(pad_editor.buttons[port][action], direction));
            pad_editor.custom[port] = TRUE;
        }
    }
    return TRUE;
}

static void pad_editor_update(struct widget_instance *list)
{
    struct widget_instance *screen = screen_of(list), *row;
    struct controller_device device;
    short port = pad_editor_port(screen);
    if (pad_editor.capturing != NONE)
    {
        int button, result = controller_remap_capture_poll(&button);
        if (result)
        {
            if (result == 1)
            {
                controller_remap_swap(pad_editor.buttons[pad_editor.port], pad_editor.capturing, button);
                pad_editor.custom[pad_editor.port] = TRUE;
            }
            pad_editor.capturing = NONE;
        }
    }
    controller_device_get(port, &device);
    for (row = list->child; row; row = row->next)
    {
        short action;
        wchar_t text[64];
        char const *label;
        short i;
        if (strncmp(row->name, "op_pad_", 7)) continue;
        action = pad_editor_action(row);
        row->visible = action != NONE;
        if (action == NONE)
        {
            if (list->focused_child == row) focus_row(list, 1);
            continue;
        }
        text_set(named(row, "pad_action", 0), pad_action_labels[action]);
        label = pad_editor.capturing == action && pad_editor.port == port ? "PRESS A BUTTON" :
            controller_remap_button_name(pad_editor.buttons[port][action], device.family);
        for (i = 0; label[i] && i < 63; i++) text[i] = (wchar_t)(unsigned char)label[i];
        text[i] = 0;
        text_set(named(row, "pad_binding", 0), text);
    }
    if (list->parameters.list.extended_description)
    {
        wchar_t text[256];
        char const *family = device.family == CONTROLLER_PLAYSTATION ? "PLAYSTATION" :
            device.family == CONTROLLER_SWITCH ? "SWITCH" : device.family == CONTROLLER_XBOX ? "XBOX" : "XBOX LABELS";
        char caption[256];
        short i;
        if (pad_editor.capturing != NONE)
            snprintf(caption, sizeof(caption), "Release, then press a button on P%d.\r\nBack / Select, Start / Options or Escape cancels.", pad_editor.port + 1);
        else
            snprintf(caption, sizeof(caption), "P%d: %s (%s). Left/right swaps buttons.\r\nSelect to bind. Defaults restores the profile layout.",
                port + 1, family, pad_editor.custom[port] ? "CUSTOM" : "PROFILE");
        for (i = 0; caption[i] && i < 255; i++) text[i] = (wchar_t)(unsigned char)caption[i];
        text[i] = 0;
        text_set_length(list->parameters.list.extended_description, text, 256);
    }
}
