/* Horizontal framing of the stock loading effect; owns no map data. */
#ifndef LOADING_WIDESCREEN_H
#define LOADING_WIDESCREEN_H

void loading_widescreen_begin(void);
int loading_widescreen_active(void);
/* the share of the loading picture's width and height left out at each
 * side so that it keeps its proportions in a frame of this aspect ratio:
 * none unless display.widescreen_loading_fill is on */
void loading_widescreen_picture_crop(float frame_aspect, float *crop_columns, float *crop_rows);
void loading_widescreen_end(void);

#endif
