/*
WEB_SHARED.H

The words of the game's memory the page reads and writes itself
(port/web/site/app.js, touch.js): the WebAssembly memory is shared, so the
page's thread sees them as the game's threads do, without a message. The
page finds them with web_state (web_main.c) and knows them by these indices,
32-bit words each: change them together.
*/

#ifndef WEB_SHARED_H
#define WEB_SHARED_H

#include <stdint.h>

enum
{
	/* the game's: the frames shown so far (the page says it is loading while
	they stand still) */
	_web_frames_shown = 0,
	/* the game's, for the touch controls: touch_input.c's _touch_scene_*
	bits, port 0's rumble (0, or 64 to 255), and the game control on each of
	the 16 controller buttons with a count of their changes */
	_web_touch_scene = 1,
	_web_touch_rumble = 2,
	_web_touch_bindings_serial = 3,
	_web_touch_bindings = 4,
	/* the page's: the controller state the touch controls make (the SDL
	axes left x and y, right x and y, the left and right triggers, then
	the SDL button bits), a count of presses of each input (the 15 buttons,
	then the triggers), and the view's swipe and the gyroscope's turn since
	the game last read them, in 1/256 of a logical pixel */
	_web_touch_state = 20,
	_web_touch_presses = 27,
	_web_touch_look = 44,
	WEB_SHARED_WORDS = 48,
};

#define WEB_TOUCH_INPUTS 17

/* the words (web_main.c) */
extern volatile int32_t web_shared[WEB_SHARED_WORDS];

#endif
