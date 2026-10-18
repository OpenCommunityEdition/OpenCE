/*
TEXT_FILTER_MATCH.C

Streamer mode's matcher: which code units of a string spell a hidden word,
however it is dressed (README.md, "Streamer mode"). A string is read as the
game draws it (draw_string.c's parsers: a wide string's "|n" breaks a line
and any other "|" hides the character after it; a narrow string's style
codes are not drawn), and each character it shows becomes an element:

- a letter, of a-z: the case folded, accents and look-alikes (Cyrillic,
  Greek, full-width, small capitals: text_filter_fold.inc) made plain;
- a digit or symbol read as letters (leet: 0 o, 1 i or l, 3 e, @ a, $ s...),
  or as any letter (* # % ?, the last of which is what the server browser's
  listings show for a character they cannot), which may also be
  punctuation;
- a space or punctuation, which a word may be spelled across;
- anything else, which no word crosses.

Two elements together may read as one letter (ph f, vv w, \/ v, |< k).
Then each term (text_filter_terms.inc, made by tools/text_filter_terms.py
from Surge AI's list; and the player's own) is looked for from each element:
a run of one letter reads as one ("fuuuck"); punctuation may come between
letters ("f.u.c.k"); spaces too where the pieces are a letter or two
("f u c k", "fu ck") or the match is whole words ("f uck"). A term's rule
says where it may be (inside a word, at a word's start, or as the whole
word), and the allowed words (Scunthorpe) are never hidden. Nothing here
knows the game: the harness builds it alone (tools/harness).
*/

#include "text_filter.h"

#include <string.h>

#define MAXIMUM_TERMS 2048
#define MAXIMUM_TERM_CHARACTERS 32768
#define MAXIMUM_TERM_LENGTH 40
#define MAXIMUM_ALLOWED 1024
#define MAXIMUM_ALLOWED_CHARACTERS 16384
#define MAXIMUM_GAP 4

#define ALL_LETTERS 0x03FFFFFFUL
#define LETTER(c) (1UL << ((c) - 'a'))

/* text_filter_terms.inc's terms: their letters with each byte xor'd by
TEXT_FILTER_TERM_KEY, so that the program holds no readable list */
struct text_filter_term_definition
{
	unsigned char group;
	unsigned char severity;
	unsigned char rule;
	char const *letters;
};

/* text_filter_fold.inc's characters read as letters */
struct text_filter_fold
{
	unsigned short code;
	unsigned long letters;
};

#include "text_filter_terms.inc"
#include "text_filter_fold.inc"

enum
{
	_element_letter,
	_element_digit,
	_element_symbol,
	_element_wildcard,
	_element_space,
	_element_punctuation,
	_element_break
};

struct element
{
	unsigned long letters;
	unsigned long pair;
	short first;
	short last;
	unsigned short code;
	unsigned char kind;
};

struct term
{
	short offset;
	unsigned char length;
	unsigned char group;
	unsigned char severity;
	unsigned char rule;
};

static struct
{
	int ready;
	long term_count;
	long character_count;
	struct term terms[MAXIMUM_TERMS];
	char characters[MAXIMUM_TERM_CHARACTERS];
	/* the terms by their first letter: first[letter] .. first[letter + 1] of
	order */
	short order[MAXIMUM_TERMS];
	short first[27];
	int sorted;
	long allowed_count;
	long allowed_character_count;
	short allowed[MAXIMUM_ALLOWED];
	char allowed_characters[MAXIMUM_ALLOWED_CHARACTERS];
} text_filter_words;

/* a term's or allowed word's letters, kept a-z only (others dropped);
their count */
static long text_filter_letters(
	char *letters,
	long size,
	char const *text)
{
	long length = 0;

	for (; *text; text++)
	{
		char character = *text;

		if (character >= 'A' && character <= 'Z')
			character = (char)(character - 'A' + 'a');
		if (character >= 'a' && character <= 'z' && length < size - 1)
			letters[length++] = character;
	}
	letters[length] = 0;
	return length;
}

static int text_filter_word_store(
	char const *letters,
	long length,
	int group,
	int severity,
	int rule)
{
	struct term *term;

	if (length < 2 || length > MAXIMUM_TERM_LENGTH ||
		text_filter_words.term_count >= MAXIMUM_TERMS ||
		text_filter_words.character_count + length + 1 > MAXIMUM_TERM_CHARACTERS ||
		group < 0 || group >= NUMBER_OF_TEXT_FILTER_GROUPS ||
		severity < _text_filter_mild || severity > _text_filter_severe ||
		rule < 0 || rule >= NUMBER_OF_TEXT_FILTER_RULES)
	{
		return 0;
	}
	term = &text_filter_words.terms[text_filter_words.term_count++];
	term->offset = (short)text_filter_words.character_count;
	term->length = (unsigned char)length;
	term->group = (unsigned char)group;
	term->severity = (unsigned char)severity;
	term->rule = (unsigned char)rule;
	memcpy(text_filter_words.characters + text_filter_words.character_count, letters, length + 1);
	text_filter_words.character_count += length + 1;
	text_filter_words.sorted = 0;
	return 1;
}

/* the built-in terms and allowed words again, without the player's */
void text_filter_words_reset(
	void)
{
	long index;

	text_filter_words.term_count = 0;
	text_filter_words.character_count = 0;
	text_filter_words.allowed_count = 0;
	text_filter_words.allowed_character_count = 0;
	for (index = 0; index < (long)(sizeof(text_filter_term_definitions) / sizeof(*text_filter_term_definitions)); index++)
	{
		struct text_filter_term_definition const *definition = &text_filter_term_definitions[index];
		char letters[MAXIMUM_TERM_LENGTH + 1];
		long length = 0;
		char const *encoded;

		for (encoded = definition->letters; *encoded && length < MAXIMUM_TERM_LENGTH; encoded++)
			letters[length++] = (char)(*encoded ^ TEXT_FILTER_TERM_KEY);
		letters[length] = 0;
		text_filter_word_store(letters, length, definition->group, definition->severity, definition->rule);
	}
	for (index = 0; index < (long)(sizeof(text_filter_allowed_words) / sizeof(*text_filter_allowed_words)); index++)
		text_filter_allow_add(text_filter_allowed_words[index]);
	text_filter_words.ready = 1;
	return;
}

/* a term of the player's, hidden at its group's level (a-z: others are
dropped); whether it was kept */
int text_filter_word_add(
	char const *term,
	int group,
	int severity,
	int rule)
{
	char letters[MAXIMUM_TERM_LENGTH + 2];
	long length;

	if (!text_filter_words.ready)
		text_filter_words_reset();
	length = text_filter_letters(letters, sizeof(letters), term);
	return text_filter_word_store(letters, length, group, severity, rule);
}

/* a word never hidden, nor any part of it (a-z); whether it was kept */
int text_filter_allow_add(
	char const *word)
{
	char letters[MAXIMUM_TERM_LENGTH + 2];
	long length = text_filter_letters(letters, sizeof(letters), word);

	if (length < 2 || length > MAXIMUM_TERM_LENGTH ||
		text_filter_words.allowed_count >= MAXIMUM_ALLOWED ||
		text_filter_words.allowed_character_count + length + 1 > MAXIMUM_ALLOWED_CHARACTERS)
	{
		return 0;
	}
	text_filter_words.allowed[text_filter_words.allowed_count++] = (short)text_filter_words.allowed_character_count;
	memcpy(text_filter_words.allowed_characters + text_filter_words.allowed_character_count, letters, length + 1);
	text_filter_words.allowed_character_count += length + 1;
	return 1;
}

/* the terms by their first letter */
static void text_filter_words_sort(
	void)
{
	long letter;
	long count = 0;

	for (letter = 0; letter < 26; letter++)
	{
		long index;

		text_filter_words.first[letter] = (short)count;
		for (index = 0; index < text_filter_words.term_count; index++)
		{
			if (text_filter_words.characters[text_filter_words.terms[index].offset] == 'a' + letter)
				text_filter_words.order[count++] = (short)index;
		}
	}
	text_filter_words.first[26] = (short)count;
	text_filter_words.sorted = 1;
	return;
}

/* ---------- elements */

/* a character above ASCII read as letters (text_filter_fold.inc), or 0 */
static unsigned long text_filter_fold_letters(
	unsigned short code)
{
	long low = 0;
	long high = (long)(sizeof(text_filter_folds) / sizeof(*text_filter_folds)) - 1;

	while (low <= high)
	{
		long middle = (low + high) / 2;

		if (text_filter_folds[middle].code == code)
			return text_filter_folds[middle].letters;
		if (text_filter_folds[middle].code < code)
			low = middle + 1;
		else
			high = middle - 1;
	}
	return 0;
}

/* what a character is (an _element_ kind; NONE (-1) for one not drawn), and
the letters it may read as */
static int text_filter_classify(
	unsigned short code,
	unsigned long *letters)
{
	*letters = 0;
	if (code >= 0xFF01 && code <= 0xFF5E)
		code = (unsigned short)(code - 0xFEE0);
	if (code >= 'A' && code <= 'Z')
		code = (unsigned short)(code - 'A' + 'a');
	if (code >= 'a' && code <= 'z')
	{
		*letters = LETTER(code);
		return _element_letter;
	}
	switch (code)
	{
	case '0': *letters = LETTER('o'); return _element_digit;
	case '1': *letters = LETTER('i') | LETTER('l'); return _element_digit;
	case '2': *letters = LETTER('z'); return _element_digit;
	case '3': *letters = LETTER('e'); return _element_digit;
	case '4': *letters = LETTER('a'); return _element_digit;
	case '5': *letters = LETTER('s'); return _element_digit;
	case '6': *letters = LETTER('g') | LETTER('b'); return _element_digit;
	case '7': *letters = LETTER('t'); return _element_digit;
	case '8': *letters = LETTER('b'); return _element_digit;
	case '9': *letters = LETTER('g') | LETTER('q'); return _element_digit;
	case '@': *letters = LETTER('a'); return _element_symbol;
	case '$': *letters = LETTER('s'); return _element_symbol;
	case '!': *letters = LETTER('i'); return _element_symbol;
	case '+': *letters = LETTER('t'); return _element_symbol;
	case '(': *letters = LETTER('c'); return _element_symbol;
	case '<': *letters = LETTER('c'); return _element_symbol;
	case '|': *letters = LETTER('i') | LETTER('l'); return _element_symbol;
	case 0x00A1: *letters = LETTER('i'); return _element_symbol;
	case 0x00A2: *letters = LETTER('c'); return _element_symbol;
	case 0x00A3: *letters = LETTER('l') | LETTER('e'); return _element_symbol;
	case 0x00A5: *letters = LETTER('y'); return _element_symbol;
	case 0x00A7: *letters = LETTER('s'); return _element_symbol;
	case 0x00A9: *letters = LETTER('c'); return _element_symbol;
	case 0x00AE: *letters = LETTER('r'); return _element_symbol;
	case 0x00D7: *letters = LETTER('x'); return _element_symbol;
	case 0x20AC: *letters = LETTER('e'); return _element_symbol;
	case '*': case '#': case '%': case '?':
		*letters = ALL_LETTERS;
		return _element_wildcard;
	case ' ': case '\t': case '\r': case '\n': case 0x00A0: case 0x1680: case 0x202F: case 0x205F: case 0x3000:
		return _element_space;
	}
	if (code >= 0x2000 && code <= 0x200A)
		return _element_space;
	/* invisible: the soft hyphen, joiners, marks over letters, direction
	and variation selectors */
	if (code == 0x00AD || code == 0x034F || code == 0x180E || code == 0xFEFF ||
		(code >= 0x0300 && code <= 0x036F) || (code >= 0x1AB0 && code <= 0x1AFF) ||
		(code >= 0x1DC0 && code <= 0x1DFF) || (code >= 0x20D0 && code <= 0x20FF) ||
		(code >= 0x200B && code <= 0x200F) || (code >= 0x202A && code <= 0x202E) ||
		(code >= 0x2060 && code <= 0x2069) || (code >= 0xFE00 && code <= 0xFE0F) ||
		(code >= 0xFE20 && code <= 0xFE2F))
	{
		return -1;
	}
	if (code >= 0x80)
	{
		*letters = text_filter_fold_letters(code);
		if (*letters)
			return _element_letter;
	}
	if ((code > ' ' && code < 0x7F) || (code >= 0x00A1 && code <= 0x00BF) ||
		(code >= 0x2010 && code <= 0x2027) || (code >= 0x2030 && code <= 0x205E))
	{
		return _element_punctuation;
	}
	return _element_break;
}

static int text_filter_style_code(
	unsigned short code)
{
	return code && code < 0x80 && strchr("ibukprlctn", (char)code) != NULL;
}

/* the string's elements as it is drawn; their count */
static long text_filter_elements(
	unsigned short const *text,
	long length,
	int narrow,
	struct element *elements,
	long maximum)
{
	long count = 0;
	long index = 0;
	long element_index;

	while (index < length && count < maximum)
	{
		unsigned short code = text[index];
		struct element *element = &elements[count];
		long last = index;
		int kind;

		if (code == '|' && index + 1 < length)
		{
			unsigned short next = text[index + 1];

			if (narrow ? text_filter_style_code(next) : 1)
			{
				last = index + 1;
				if (next == 'n' || (narrow && next == 't'))
				{
					element->letters = 0;
					element->kind = _element_space;
					element->code = ' ';
					element->first = (short)index;
					element->last = (short)last;
					count++;
					index = last + 1;
					continue;
				}
				if (narrow)
				{
					/* a style code: not drawn */
					index = last + 1;
					continue;
				}
				/* a wide string's "|": drawn, the character after it not */
			}
		}
		kind = text_filter_classify(code, &element->letters);
		if (kind < 0)
		{
			index++;
			continue;
		}
		element->kind = (unsigned char)kind;
		element->code = code >= 'A' && code <= 'Z' ? (unsigned short)(code - 'A' + 'a') : code;
		element->first = (short)index;
		element->last = (short)last;
		count++;
		index = last + 1;
	}

	/* pairs read as one letter */
	for (element_index = 0; element_index < count; element_index++)
	{
		unsigned short first = elements[element_index].code;
		unsigned short second = element_index + 1 < count ? elements[element_index + 1].code : 0;
		unsigned long pair = 0;

		if (first == 'p' && second == 'h') pair = LETTER('f');
		else if (first == 'v' && second == 'v') pair = LETTER('w');
		else if (first == '\\' && second == '/') pair = LETTER('v');
		else if (first == '/' && second == '\\') pair = LETTER('a');
		else if (first == '|' && second == '<') pair = LETTER('k');
		else if (first == '>' && second == '<') pair = LETTER('x');
		else if (first == '(' && second == ')') pair = LETTER('o');
		else if (first == '[' && second == ']') pair = LETTER('o');
		else if (first == '|' && second == ')') pair = LETTER('d');
		elements[element_index].pair = pair;
	}
	return count;
}

/* ---------- matching */

struct attempt
{
	struct element const *elements;
	long count;
	char const *letters;
	long length;
	int rule;
	long start;
	long end;
};

/* what a match has crossed so far */
struct path
{
	long wildcards;
	long real_letters;
	long piece;
	long longest_piece;
	int spaced;
	int crossed;
};

static int text_filter_try_letter(struct attempt *attempt, long term_index, long element_index, struct path path);

static int text_filter_word_character(
	struct element const *element)
{
	return element->kind == _element_letter;
}

/* whether an allowed word covers the match, inside the word it is in */
static int text_filter_allowed(
	struct attempt const *attempt)
{
	long word_start = attempt->start;
	long word_end = attempt->end;
	long index;

	while (word_start > 0 &&
		(attempt->elements[word_start - 1].kind == _element_letter || attempt->elements[word_start - 1].kind == _element_digit))
	{
		word_start--;
	}
	while (word_end < attempt->count &&
		(attempt->elements[word_end].kind == _element_letter || attempt->elements[word_end].kind == _element_digit))
	{
		word_end++;
	}
	for (index = 0; index < text_filter_words.allowed_count; index++)
	{
		char const *allowed = text_filter_words.allowed_characters + text_filter_words.allowed[index];
		long length = (long)strlen(allowed);
		long at;

		for (at = word_start; at + length <= word_end; at++)
		{
			long offset;

			if (at > attempt->start || at + length < attempt->end)
				continue;
			for (offset = 0; offset < length; offset++)
			{
				if (!(attempt->elements[at + offset].letters & LETTER(allowed[offset])))
					break;
			}
			if (offset == length)
				return 1;
		}
	}
	return 0;
}

/* the term read to the element before end: whether the match stands */
static int text_filter_finish(
	struct attempt *attempt,
	long end,
	struct path path)
{
	struct element const *elements = attempt->elements;
	int start_boundary = attempt->start == 0 || !text_filter_word_character(&elements[attempt->start - 1]);
	int end_boundary = end >= attempt->count || !text_filter_word_character(&elements[end]);

	if (path.piece > path.longest_piece)
		path.longest_piece = path.piece;
	/* any letter (* ? #) once in a short word, twice in a longer one, and
	never in a word of three letters */
	if (path.wildcards && (attempt->length < 4 || path.wildcards > (attempt->length <= 5 ? 1 : 2)))
		return 0;
	/* a third of the letters letters, not digits or symbols (not "8008") */
	if (!path.real_letters || path.real_letters * 3 < attempt->length)
		return 0;
	/* across spaces: pieces of a letter or two, or whole words */
	if (path.spaced && path.longest_piece > 2 && !(start_boundary && end_boundary))
		return 0;
	switch (attempt->rule)
	{
	case _text_filter_start:
		if (!start_boundary)
			return 0;
		break;
	case _text_filter_whole:
		if (!start_boundary)
			return 0;
		if (!end_boundary)
		{
			/* or with "s", or "es" after s, x, z, ch or sh */
			long after = end;
			char last = attempt->letters[attempt->length - 1];
			char before_last = attempt->length > 1 ? attempt->letters[attempt->length - 2] : 0;

			if ((last == 's' || last == 'x' || last == 'z' || (last == 'h' && (before_last == 'c' || before_last == 's'))) &&
				after < attempt->count && (elements[after].letters & LETTER('e')) && elements[after].kind == _element_letter &&
				after + 1 < attempt->count && (elements[after + 1].letters & LETTER('s')))
			{
				after++;
			}
			if (after < attempt->count && (elements[after].letters & LETTER('s')) &&
				(elements[after].kind == _element_letter || elements[after].kind == _element_digit) &&
				(after + 1 >= attempt->count || !text_filter_word_character(&elements[after + 1])))
			{
				end = after + 1;
			}
			else
			{
				return 0;
			}
		}
		break;
	}
	if (!path.crossed)
	{
		attempt->end = end;
		if (text_filter_allowed(attempt))
			return 0;
	}
	attempt->end = end;
	return 1;
}

/* the term's letter at term_index from element_index on, after what came
before (spaces and punctuation between letters skipped, at most
MAXIMUM_GAP) */
static int text_filter_next(
	struct attempt *attempt,
	long term_index,
	long element_index,
	long gap,
	int spaced,
	struct path path)
{
	struct element const *element;

	if (term_index == attempt->length)
		return text_filter_finish(attempt, element_index, path);
	if (element_index >= attempt->count)
		return 0;
	element = &attempt->elements[element_index];
	if (gap > 0)
	{
		path.crossed = 1;
		if (spaced)
		{
			path.spaced = 1;
			if (path.piece > path.longest_piece)
				path.longest_piece = path.piece;
			path.piece = 0;
		}
	}
	if ((element->letters || element->pair) && text_filter_try_letter(attempt, term_index, element_index, path))
		return 1;
	if (term_index > 0 && gap < MAXIMUM_GAP &&
		(element->kind == _element_space || element->kind == _element_punctuation ||
		element->kind == _element_symbol || element->kind == _element_wildcard))
	{
		return text_filter_next(attempt, term_index, element_index + 1, gap + 1,
			spaced || element->kind == _element_space, path);
	}
	return 0;
}

/* after the term's letter at term_index, read to element end: the same
letter again ("fuuuck") is part of it, unless the term has it twice; for a
short whole word, only three or more of it ("woops" is not a short word's
"o" twice) */
static int text_filter_after(
	struct attempt *attempt,
	long term_index,
	long end,
	struct path path)
{
	char letter = attempt->letters[term_index];

	if (term_index + 1 >= attempt->length || attempt->letters[term_index + 1] != letter)
	{
		long repeated = end;

		while (repeated < attempt->count &&
			(attempt->elements[repeated].kind == _element_letter || attempt->elements[repeated].kind == _element_digit) &&
			(attempt->elements[repeated].letters & LETTER(letter)))
		{
			repeated++;
		}
		if (repeated != end &&
			(attempt->rule != _text_filter_whole || attempt->length > 4 || repeated - end >= 2))
		{
			struct path longer = path;

			longer.piece += repeated - end;
			if (text_filter_next(attempt, term_index + 1, repeated, 0, 0, longer))
				return 1;
		}
	}
	return text_filter_next(attempt, term_index + 1, end, 0, 0, path);
}

static int text_filter_try_letter(
	struct attempt *attempt,
	long term_index,
	long element_index,
	struct path path)
{
	struct element const *element = &attempt->elements[element_index];
	unsigned long letter = LETTER(attempt->letters[term_index]);

	if (element->letters & letter)
	{
		struct path one = path;

		one.piece++;
		if (element->kind == _element_wildcard)
			one.wildcards++;
		else if (element->kind == _element_letter)
			one.real_letters++;
		if ((element->kind != _element_wildcard || term_index > 0) &&
			text_filter_after(attempt, term_index, element_index + 1, one))
		{
			return 1;
		}
	}
	if (element->pair & letter)
	{
		struct path two = path;

		two.piece++;
		if (element->kind == _element_letter)
			two.real_letters++;
		if (text_filter_after(attempt, term_index, element_index + 2, two))
			return 1;
	}
	return 0;
}

/* which code units of the text (of a wide string, or narrow: char's widened)
spell a term of a group whose level its severity reaches (levels: one for
each group, _text_filter_off or the least severity hidden), marked in
censored (as long as the text); how many */
long text_filter_censor(
	unsigned short const *text,
	long length,
	int narrow,
	unsigned char const *levels,
	unsigned char *censored)
{
	static struct element elements[TEXT_FILTER_MAXIMUM_LENGTH];
	long count;
	long start;
	long marked = 0;

	memset(censored, 0, length);
	if (!text_filter_words.ready)
		text_filter_words_reset();
	if (!text_filter_words.sorted)
		text_filter_words_sort();
	count = text_filter_elements(text, length, narrow, elements, TEXT_FILTER_MAXIMUM_LENGTH);
	for (start = 0; start < count; start++)
	{
		unsigned long letters = elements[start].kind == _element_wildcard ? 0 : (elements[start].letters | elements[start].pair);
		long letter;

		for (letter = 0; letters && letter < 26; letter++)
		{
			long order;

			if (!(letters & (1UL << letter)))
				continue;
			for (order = text_filter_words.first[letter]; order < text_filter_words.first[letter + 1]; order++)
			{
				struct term const *term = &text_filter_words.terms[text_filter_words.order[order]];
				struct attempt attempt;
				struct path path;

				if (levels[term->group] == _text_filter_off || term->severity < levels[term->group])
					continue;
				attempt.elements = elements;
				attempt.count = count;
				attempt.letters = text_filter_words.characters + term->offset;
				attempt.length = term->length;
				attempt.rule = term->rule;
				attempt.start = start;
				attempt.end = start;
				memset(&path, 0, sizeof(path));
				if (text_filter_try_letter(&attempt, 0, start, path))
				{
					long unit;

					for (unit = elements[start].first; unit <= elements[attempt.end - 1].last; unit++)
					{
						if (!censored[unit])
						{
							censored[unit] = 1;
							marked++;
						}
					}
				}
			}
		}
	}
	return marked;
}
