#ifndef MCC_MENU_PREVIEW_H
#define MCC_MENU_PREVIEW_H
struct rasterizer_dynamic_screen_geometry_parameters;
union rectangle2d;
/* Only the MCC display range on the level-picture tag is intercepted. */
short mcc_menu_preview_index(long bitmap_tag, short frame);
boolean mcc_menu_preview_draw(short index, union rectangle2d const *bounds, union rectangle2d const *clip,
    unsigned long color, struct rasterizer_dynamic_screen_geometry_parameters *parameters);
void mcc_menu_preview_clear(void);
#endif
