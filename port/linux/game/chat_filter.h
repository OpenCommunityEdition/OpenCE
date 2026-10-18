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
/* (draw_string.c) a text the game draws or measures as streamer mode shows
it: the string itself when streamer mode is off, else the text with its words
replaced (its format codes kept), valid until the next call. So players'
names show replaced wherever they are drawn: the scoreboard, above their
heads, the kills, the lobby, the chat */
wchar_t const *chat_filter_shown(wchar_t const *string);
char const *chat_filter_shown_ascii(char const *string);

#endif // __CHAT_FILTER_H
