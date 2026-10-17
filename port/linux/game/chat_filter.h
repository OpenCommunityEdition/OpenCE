/*
CHAT_FILTER.H

Streamer mode's word filter (chat_filter.c): what the text chat
(network_chat.c) asks of it.
*/

#ifndef __CHAT_FILTER_H
#define __CHAT_FILTER_H
#pragma once

/* ---------- prototypes/CHAT_FILTER.C */

/* whether streamer mode is on (game.streamer_mode) */
boolean chat_filter_enabled(void);
/* the text with the words streamer mode replaces replaced, at most size
characters with its end; its length */
short chat_filter_text(wchar_t *destination, short size, wchar_t const *source);

#endif // __CHAT_FILTER_H
