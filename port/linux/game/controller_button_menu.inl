/* Included by menu_functions.c: pending spinners remain independent, so Cancel,
 * Defaults and family changes never overwrite the saved manual preference. */
static void controller_button_rows_show(struct widget_instance *list)
{
    static char const *rows[] = {"op_hud_button_style", "op_hud_button_style_xbox",
        "op_hud_button_style_ps", "op_hud_button_style_switch"};
    static char const *spinners[] = {"hud_button_style_spinner", "hud_button_style_xbox_spinner",
        "hud_button_style_ps_spinner", "hud_button_style_switch_spinner"};
    struct widget_instance *automatic = named(list, "auto_button_detection_spinner", 0);
    struct widget_instance *bumper = named(list, "op_hud_bumper_style", 0);
    struct widget_instance *shown, *row, *child, *label, *spinner;
    struct pc_menu_setting *setting;
    struct controller_device device;
    short index, family = 0, selected, port;
    boolean bumper_visible = FALSE;
    wchar_t caption[64];
    if (!automatic) return;
    port = controller_of(list);
    controller_device_get(port, &device);
    setting = pc_menu_setting_get(automatic->definition_tag_index);
    index = automatic->parameters.list.selected_index;
    if (setting && index >= 0 && index < setting->value_count && !strcmp(setting->values[index], "true"))
        family = (short)device.family;
    if (family < 0 || family > 3) family = 0;
    shown = named(list, rows[family], 0);
    if (!shown) return;
    for (index = 0; index < 4; index++)
    {
        row = named(list, rows[index], 0);
        if (!row) continue;
        row->visible = row == shown;
        if (!row->visible && list->focused_child == row) list->focused_child = shown;
    }
    if (bumper)
    {
        spinner = named(list, spinners[family], 0);
        setting = spinner ? pc_menu_setting_get(spinner->definition_tag_index) : NULL;
        index = spinner ? spinner->parameters.list.selected_index : -1;
        if (setting && index >= 0 && index < setting->value_count)
            bumper_visible = controller_button_style_index(setting->values[index]) < 3;
        /* Last option in Button Appearance; help and bottom buttons stay put. */
        bumper->visible = bumper_visible;
        if (!bumper_visible && list->focused_child == bumper) list->focused_child = shown;
    }
    for (selected = 0, child = list->child; child; child = child->next, selected++)
        if (child == list->focused_child) list->parameters.list.selected_index = selected;
    label = shown->child;
    if (label)
    {
        usnprintf(caption, NUMBEROF(caption), L"BUTTON STYLE (P%d):", port + 1);
        text_set(label, caption);
    }
}
