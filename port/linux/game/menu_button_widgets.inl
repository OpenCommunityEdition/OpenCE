/* Included by menu_functions.c: PC glyph layout uses its existing controller
 * context without changing the widget definitions or input routing. */
/* Only the PC join screen reuses this Xbox-authored label in a wider panel.
 * Supply its parent's horizontal bounds; the feature owns mixed-run layout. */
void pc_menu_widget_text_bounds(long tag, rectangle2d *bounds, short *text_offset);

/* The compact pause footer has room after SELECT, but little before CANCEL.
 * Recover its spare gap and balance the added width around its center. Full-size
 * legends instead have spare space on their left. Shift the whole widget so
 * its text and mouse target follow the keycap. */
short menu_functions_glyph_offset(struct widget_instance *widget)
{
    struct widget_instance *sibling;
    char const *name, *leaf;
    rectangle2d bounds;
    short text_offset, extra = 0, balance = 0, width, button, accept_x = 0, label_end = 0, gap = 0;
    boolean compact;
    if (!widget->parent || !menu_button_glyph_keyboard_active(controller_of(widget))) return 0;
    if (menu_button_glyph_key_width(0) <= 0) return 0;
    if (mcc_pause_owns(widget->parent->definition_tag_index))
        return mcc_pause_key_offset(widget->parent->definition_tag_index,
            widget->definition_tag_index, menu_button_glyph_compact_key_width(0), menu_button_glyph_compact_key_width(1));
    name = tag_get_name(widget->parent->definition_tag_index);
    leaf = name ? strrchr(name, '\\') : NULL;
    if (!leaf || (strcmp(leaf, "\\button_key") && strcmp(leaf, "\\button_key_options") &&
        strcmp(leaf, "\\mp_button_key") && strcmp(leaf, "\\button_key_sm") &&
        strcmp(leaf, "\\confirm_buttons"))) return 0;
    compact = !strcmp(leaf, "\\button_key_sm");
    /* Confirmation legends use confirm_buttons while containing the same
     * small face-button slots as the pause footer. Match their authored slots. */
    for (sibling = widget->parent->child; sibling; sibling = sibling->next)
    {
        name = tag_get_name(sibling->definition_tag_index);
        leaf = name ? strrchr(name, '\\') : NULL;
        if (leaf && !strcmp(leaf, "\\a_butn_sm")) compact = TRUE;
    }
    if (compact)
    {
        /* These groups have varying authored gaps. Recover spare
         * space or add missing space, keeping eight units after CANCEL. */
        for (sibling = widget->parent->child; sibling; sibling = sibling->next)
        {
            name = tag_get_name(sibling->definition_tag_index);
            leaf = name ? strrchr(name, '\\') : NULL;
            if (leaf && !strcmp(leaf, "\\a_butn_sm")) accept_x = sibling->horizontal_offset;
        }
        for (sibling = widget->parent->child; sibling; sibling = sibling->next)
        {
            name = tag_get_name(sibling->definition_tag_index);
            leaf = name ? strrchr(name, '\\') : NULL;
            if (!leaf || leaf[1] != '=' || sibling->horizontal_offset >= accept_x) continue;
            pc_menu_widget_text_bounds(sibling->definition_tag_index, &bounds, &text_offset);
            label_end = MAX(label_end, sibling->horizontal_offset + bounds.x1);
        }
        if (label_end) gap = accept_x - label_end - 8;
    }
    for (sibling = widget->parent->child; sibling; sibling = sibling->next)
    {
        name = tag_get_name(sibling->definition_tag_index);
        leaf = name ? strrchr(name, '\\') : NULL;
        if (!leaf) continue;
        button = (!strcmp(leaf, "\\a_butn") || !strcmp(leaf, "\\a_butn_sm")) ? 0 :
            (!strcmp(leaf, "\\b_butn") || !strcmp(leaf, "\\b_butn_sm")) ? 1 :
            (!strcmp(leaf, "\\x_butn") || !strcmp(leaf, "\\x_butn_sm")) ? 2 :
            (!strcmp(leaf, "\\y_butn") || !strcmp(leaf, "\\y_butn_sm")) ? 3 : NONE;
        if (button == NONE) continue;
        width = compact ? menu_button_glyph_compact_key_width(button) : menu_button_glyph_key_width(button);
        pc_menu_widget_text_bounds(sibling->definition_tag_index, &bounds, &text_offset);
        width = MAX(0, width - (bounds.x1 - bounds.x0));
        /* Compact footers are B/CANCEL followed by A/SELECT (or A/OK).
         * Balance the extra width on both ends around the authored center. */
        if (compact && button == 0) width -= gap;
        if (compact) balance += button == 0 ? width : button == 1 ? -width : 0;
        if (compact ? button != 0 || sibling->horizontal_offset > widget->horizontal_offset :
            sibling->horizontal_offset <= widget->horizontal_offset) continue;
        extra += width;
    }
    return compact ? extra - balance / 2 : -extra;
}

/* MCC owns its generated key items. Only keyboard mode asks it for their
 * identities; native controller drawing and the Xbox menu choice stay intact. */
short menu_functions_glyph_button(struct widget_instance *widget, short native)
{
    if (native == NONE && widget->parent &&
        menu_button_glyph_keyboard_active(controller_of(widget)))
        return mcc_pause_key_button(widget->parent->definition_tag_index, widget->definition_tag_index);
    return native;
}

boolean menu_functions_glyph_text(struct widget_instance *widget, rectangle2d const *bounds)
{
    rectangle2d definition, parent;
    short text_offset, ignored;
    rectangle2d panel = *bounds;
    char const *name, *parent_name;
    if (!widget->parent) return FALSE;
    name = tag_get_name(widget->definition_tag_index);
    parent_name = tag_get_name(widget->parent->definition_tag_index);
    if (!name || !parent_name ||
        !strstr(name, "\\press_start_to_join") ||
        !strstr(parent_name, "\\lobby\\join4_qtr_join")) return FALSE;
    pc_menu_widget_text_bounds(widget->definition_tag_index, &definition, &text_offset);
    pc_menu_widget_text_bounds(widget->parent->definition_tag_index, &parent, &ignored);
    panel.x0 = bounds->x0 - definition.x0 - text_offset -
        widget->horizontal_offset + parent.x0;
    panel.x1 = panel.x0 + parent.x1 - parent.x0;
    return menu_button_glyph_join_draw(widget->parameters.text_box.text, controller_of(widget), &panel);
}

