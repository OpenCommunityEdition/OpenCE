#ifndef MCC_MENU_TEXT_H
#define MCC_MENU_TEXT_H
void mcc_menu_text_row(wchar_t text[64]);
boolean mcc_menu_text_draw(long widget_tag, short display_index, wchar_t const *text,
    long font, short justification, real_argb_color const *color,
    rectangle2d const *bounds, rectangle2d const *clip);
#endif
