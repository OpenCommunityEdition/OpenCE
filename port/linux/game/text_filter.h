/*
TEXT_FILTER.H

Streamer mode (Settings > Network Setup > Streamer Mode, streamer.*): the
words hidden as stars in what is drawn. text_filter_match.c finds them, with
no game types, so that the harness builds it alone
(tools/harness/tests/test_text_filter.py); text_filter.c applies them to
what the game draws.
*/

#ifndef __TEXT_FILTER_H__
#define __TEXT_FILTER_H__

#include <stddef.h>

/* the groups a player sets a level for, and the player's own words
(text_filter.txt), hidden whenever streamer mode is on */
enum text_filter_group
{
	_text_filter_slurs,
	_text_filter_swearing,
	_text_filter_insults,
	_text_filter_own,
	NUMBER_OF_TEXT_FILTER_GROUPS
};

/* a term's severity (Surge AI's: mild, strong, severe), and a group's
level: the least severity hidden, or none */
enum text_filter_severity
{
	_text_filter_off,
	_text_filter_mild,
	_text_filter_strong,
	_text_filter_severe
};

/* where a term may be found: inside any word, at a word's start, or as the
whole word (or it and "s" or "es") */
enum text_filter_rule
{
	_text_filter_anywhere,
	_text_filter_start,
	_text_filter_whole,
	NUMBER_OF_TEXT_FILTER_RULES
};

/* the most code units of a string looked at (a longer one is drawn as it is) */
#define TEXT_FILTER_MAXIMUM_LENGTH 1024

/* text_filter_match.c */
void text_filter_words_reset(void);
int text_filter_word_add(char const *term, int group, int severity, int rule);
int text_filter_allow_add(char const *word);
long text_filter_censor(unsigned short const *text, long length, int narrow, unsigned char const *levels,
	unsigned char *censored);

/* text_filter.c */
wchar_t const *text_filter_unicode(wchar_t const *string);
char const *text_filter_string(char const *string);
void text_filter_suspend(void);
void text_filter_resume(void);
void text_filter_exempt(void const *text);
void text_filter_utf8(char *text);

#endif
