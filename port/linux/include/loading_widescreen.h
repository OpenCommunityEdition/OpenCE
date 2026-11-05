/* Optional presentation of the stock loading texture; owns no map data. */
#ifndef LOADING_WIDESCREEN_H
#define LOADING_WIDESCREEN_H

/* stock_texture is the progress renderer's IDirect3DTexture8. */
int loading_widescreen_draw(float progress, void *stock_texture);
void loading_widescreen_end(void);

#endif
