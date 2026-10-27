/* Included by menu_functions.c: pending spinners remain independent, so Cancel,
 * Defaults and family changes never overwrite the saved manual preference. */
static void controller_button_rows_show(struct widget_instance *list)
{
    static char const *rows[] = {"op_hud_button_style", "op_hud_button_style_xbox",
        "op_hud_button_style_ps", "op_hud_button_style_switch"};
    struct widget_instance *hud = named(list, "high_res_hud_spinner", 0);
    struct widget_instance *automatic = named(list, "auto_button_detection_spinner", 0);
    struct widget_instance *auto_row = named(list, "op_auto_button_detection", 0);
    struct widget_instance *manual = named(list, rows[0], 0);
    struct widget_instance *shown, *row, *child, *label;
    struct pc_menu_setting *setting;
    struct controller_device device;
    short index, family = 0, selected, port;
    boolean visible;
    wchar_t caption[64];
    if (!hud || !automatic || !auto_row || !manual) return;
    setting = pc_menu_setting_get(hud->definition_tag_index);
    index = hud->parameters.list.selected_index;
    visible = setting && index >= 0 && index < setting->value_count && !strcmp(setting->values[index], "true");
    port = controller_of(list);
    controller_device_get(port, &device);
    setting = pc_menu_setting_get(automatic->definition_tag_index);
    index = automatic->parameters.list.selected_index;
    if (setting && index >= 0 && index < setting->value_count && !strcmp(setting->values[index], "true"))
        family = (short)device.family;
    if (family < 0 || family > 3) family = 0;
    shown = named(list, rows[family], 0);
    /* Both optional rows occupy 27 pixels; family rows share one position. */
    if (auto_row->visible != visible)
    {
        row = named(list, rows[3], 0);
        for (child = row ? row->next : manual->next; child; child = child->next)
            if (!strncmp(child->name, "op_", 3))
                child->vertical_offset += visible ? 54 : -54;
        auto_row->visible = visible;
    }
    for (index = 0; index < 4; index++)
    {
        row = named(list, rows[index], 0);
        if (!row) continue;
        row->visible = visible && row == shown;
        if (!row->visible && list->focused_child == row) list->focused_child = visible ? shown : hud->parent;
    }
    if (!visible && list->focused_child == auto_row) list->focused_child = hud->parent;
    for (selected = 0, child = list->child; child; child = child->next, selected++)
        if (child == list->focused_child) list->parameters.list.selected_index = selected;
    label = shown ? shown->child : NULL;
    if (label)
    {
        usnprintf(caption, NUMBEROF(caption), L"BUTTON STYLE (P%d):", port + 1);
        text_set(label, caption);
    }
}
