/*
CHAT_FILTER.C

Streamer mode (game.streamer_mode, off by default): the text the game
draws shows the slurs and swearing in it as harmless words instead, the
chat's lines and players' names wherever they show, so that what a stream
shows stays clean. It changes only what this machine draws (draw_string.c,
through chat_filter_shown): every machine is sent the lines and names as
they were typed, and debug.txt keeps them so.

Each word of the list matches in either case, and in the spellings that
dodge a plain list:

- digits and signs for the letters they look like ("n1gg3r", "$hit",
  "c00n"), l for i ("nlgga"), q for g ("niqqa"), k for c ("kunt"), v for u
  ("fvck"), ph for f ("phaggot"), and * for a vowel ("f*ck");
- a letter typed more than once ("fuuuck", "niggggaaa");
- a word spelled out, a separator between each of its letters
  ("n i g g e r", "c.u.n.t");
- endings typed more than once ("niggazzz").

Where a word may stand, so that harmless words that hold one are left alone
(Scunthorpe, snigger, Niger, spicy, raccoon, cocky, assassin, therapist):

- a word alone, but for a plural's ending (spic, spics);
- ... or a few endings more (retard, retarded, retards; not retardant);
- the start of a word, whatever follows (cunt, cunts, cuntish);
- anywhere (fuck: motherfucker).

The harmless word takes the case of the one it replaces (Fuck: Fudge, FUCK:
FUDGE) and the ending after it, fitted to it (fucking: fudging, bitches:
biscuits, faggots: flurries).
*/

#include "cseries.h"
#include "chat_filter.h"

/* the platform layer's (port/linux/src/port_config.c) */
int config_boolean(char const *name);
unsigned long config_changes(void);

/* ---------- constants */

/* where a word may stand */
enum
{
	/* alone, or with a plural's ending */
	_chat_filter_word,
	/* ... or a few endings more */
	_chat_filter_word_endings,
	/* at the start of a word */
	_chat_filter_word_start,
	_chat_filter_anywhere,
};

enum
{
	/* the endings of _chat_filter_word (the first of chat_filter_endings) */
	CHAT_FILTER_PLURAL_ENDINGS = 4,
	/* the texts drawn (chat_filter_shown): those remembered, filtered, and how
	long one may be (a longer one is drawn as it is); how much longer a text
	may grow (kys: keep yourself safe); a run between format codes at most */
	CHAT_FILTER_CACHED = 48,
	CHAT_FILTER_CACHED_SIZE = 256,
	CHAT_FILTER_GROWTH = 4,
	CHAT_FILTER_RUN_SIZE = 256,
};

/* ---------- structures */

struct chat_filter_word
{
	/* in lower case; a space matches one or more */
	char const *word;
	/* NULL: kept as typed (a harmless word that holds one of the list's) */
	char const *replacement;
	short match;
};

/* ---------- globals */

/* (the longer before the words they hold, a word's spellings together) */
static struct chat_filter_word const chat_filter_words[] =
{
	/* racist slurs and phrases */
	/* (kept as they are: the harmless words that hold the n-word's letters) */
	{ "snigg", NULL, _chat_filter_word_start },
	{ "niggl", NULL, _chat_filter_word_start },
	{ "niggard", NULL, _chat_filter_word_start },
	{ "nigger", "nugget", _chat_filter_word_start },
	{ "niglet", "piglet", _chat_filter_anywhere },
	{ "nigette", "nugget", _chat_filter_word_start },
	{ "nigr", "noodle", _chat_filter_word_start },
	{ "nignog", "eggnog", _chat_filter_word_start },
	{ "niga", "ninja", _chat_filter_word },
	{ "nigah", "ninja", _chat_filter_word },
	{ "niguh", "ninja", _chat_filter_word },
	{ "nigor", "noodle", _chat_filter_word },
	{ "nigar", "noodle", _chat_filter_word },
	{ "nigur", "noodle", _chat_filter_word },
	{ "nigg", "nugget", _chat_filter_anywhere },
	{ "nig", "noodle", _chat_filter_word },
	{ "kneegrow", "knee pad", _chat_filter_word_start },
	{ "knee grow", "knee pad", _chat_filter_word },
	{ "jigaboo", "jigglypuff", _chat_filter_word_start },
	{ "porch monkey", "porch swing", _chat_filter_word },
	{ "jungle monkey", "jungle gym", _chat_filter_word },
	{ "melon monkey", "melon baller", _chat_filter_word },
	{ "jungle bunny", "easter bunny", _chat_filter_word },
	{ "moon cricket", "moon pie", _chat_filter_word },
	{ "mooncricket", "moon pie", _chat_filter_word_start },
	{ "spear chucker", "pea shooter", _chat_filter_word },
	{ "tar baby", "baby yoda", _chat_filter_word },
	{ "burrhead", "bed head", _chat_filter_word_start },
	{ "burr head", "bed head", _chat_filter_word },
	{ "diaper head", "diaper bag", _chat_filter_word },
	{ "gator bait", "gatorade", _chat_filter_word },
	{ "christ killer", "chris pratt", _chat_filter_word },
	{ "klansm", "clansm", _chat_filter_word_start },
	{ "klansw", "clansw", _chat_filter_word_start },
	{ "pickaninny", "picnic basket", _chat_filter_word_start },
	{ "golliwog", "golly gosh", _chat_filter_word_start },
	{ "sambo", "samba", _chat_filter_word },
	{ "darkie", "doggie", _chat_filter_word },
	{ "darky", "doggy", _chat_filter_word },
	{ "darkey", "doggy", _chat_filter_word },
	{ "wigger", "wiggler", _chat_filter_word_endings },
	{ "africoon", "afro puff", _chat_filter_word },
	{ "coonass", "cocoon", _chat_filter_word_start },
	{ "coon", "cocoon", _chat_filter_word },
	{ "wog", "walrus", _chat_filter_word },
	{ "lynch", "high five", _chat_filter_word_endings },
	{ "chink", "chipmunk", _chat_filter_word_endings },
	{ "chinaman", "china plate", _chat_filter_word },
	{ "ching chong", "ding dong", _chat_filter_word },
	{ "gook", "goose", _chat_filter_word },
	{ "zipperhead", "zipper bag", _chat_filter_word_start },
	{ "jape", NULL, _chat_filter_word_start },
	{ "jap", "jalapeno", _chat_filter_word },
	{ "spice", NULL, _chat_filter_word_start },
	{ "spic", "spork", _chat_filter_word },
	{ "spick", "spork", _chat_filter_word },
	{ "tacohead", "taco tuesday", _chat_filter_word_start },
	{ "wetback", "waffle", _chat_filter_word_start },
	{ "beaner", "bean bag", _chat_filter_word },
	{ "kike", "kite", _chat_filter_word },
	{ "yid", "yeti", _chat_filter_word },
	{ "heeb", "herb", _chat_filter_word },
	{ "jewed", "chewed", _chat_filter_word },
	{ "jewboy", "cowboy", _chat_filter_word_start },
	{ "hymie", "hammy", _chat_filter_word },
	{ "towelhead", "beach towel", _chat_filter_word_start },
	{ "raghead", "rag doll", _chat_filter_word_start },
	{ "camel jockey", "disc jockey", _chat_filter_word },
	{ "greaseball", "meatball", _chat_filter_word_start },
	{ "woop", NULL, _chat_filter_word_start },
	{ "wop", "whopper", _chat_filter_word },
	{ "dago", "dango", _chat_filter_word },
	{ "coolie", "cookie", _chat_filter_word },
	{ "boche", "brioche", _chat_filter_word },
	{ "boonga", "bongo", _chat_filter_word },
	{ "abbo", "avocado", _chat_filter_word },
	{ "kaffir", "coffee", _chat_filter_word },
	{ "kaffer", "coffee", _chat_filter_word },
	{ "kraut", "sauerkraut", _chat_filter_word },
	{ "paki", "pancake", _chat_filter_word },
	/* homophobic and transphobic */
	{ "fagot", "flurry", _chat_filter_word_start },
	{ "faget", "flurry", _chat_filter_word },
	{ "fagit", "flurry", _chat_filter_word },
	{ "faq", NULL, _chat_filter_word_start },
	{ "phage", NULL, _chat_filter_word_start },
	{ "fag", "flapjack", _chat_filter_word },
	{ "feg", "flapjack", _chat_filter_word },
	{ "carpet muncher", "carpet cleaner", _chat_filter_word },
	{ "carpetmuncher", "carpet cleaner", _chat_filter_word_start },
	{ "fudge packer", "fudge maker", _chat_filter_word },
	{ "fudgepacker", "fudge maker", _chat_filter_word_start },
	{ "pillow biter", "pillow fighter", _chat_filter_word },
	{ "lesbo", "lasso", _chat_filter_word },
	{ "lezbo", "lasso", _chat_filter_word },
	{ "lezie", "lizzie", _chat_filter_word },
	{ "lezy", "lizzy", _chat_filter_word },
	{ "homo", "hobo", _chat_filter_word },
	{ "dyke", "duckling", _chat_filter_word },
	{ "tranies", "trampolines", _chat_filter_word },
	{ "trany", "trampoline", _chat_filter_word },
	{ "shemale", "she-hulk", _chat_filter_word_start },
	/* ableist */
	{ "retard", "rutabaga", _chat_filter_word_endings },
	{ "tard", "tater", _chat_filter_word },
	{ "spaz", "spud", _chat_filter_word },
	{ "space", NULL, _chat_filter_word_start },
	{ "spac", "spud", _chat_filter_word },
	{ "mongoloid", "mongoose", _chat_filter_word_start },
	/* hate */
	{ "neonazi", "neo nacho", _chat_filter_word_start },
	{ "nazi", "nacho", _chat_filter_word_start },
	{ "hitler", "hamster", _chat_filter_word },
	{ "sieg heil", "see ya later", _chat_filter_word },
	{ "heil", "hail", _chat_filter_word },
	{ "kkk", "kfc", _chat_filter_word },
	{ "1488", "1234", _chat_filter_word },
	/* telling someone to die, and worse */
	{ "kill yourself", "treat yourself", _chat_filter_word },
	{ "kill your self", "treat your self", _chat_filter_word },
	{ "kill urself", "treat urself", _chat_filter_word },
	{ "neck yourself", "check yourself", _chat_filter_word },
	{ "hang yourself", "hug yourself", _chat_filter_word },
	{ "kys", "keep yourself safe", _chat_filter_word },
	{ "rapist", "grape farmer", _chat_filter_word },
	{ "rapp", NULL, _chat_filter_word_start },
	{ "daterape", "date grape", _chat_filter_word_start },
	{ "rape", "grape", _chat_filter_word_endings },
	{ "pedophile", "pedal pusher", _chat_filter_word_start },
	{ "pedo", "pedalo", _chat_filter_word },
	/* swearing */
	{ "cocksuck", "lollipop lick", _chat_filter_word_start },
	/* (kept as it is: the town) */
	{ "scunthorpe", NULL, _chat_filter_word },
	{ "cunt", "chop stick", _chat_filter_anywhere },
	{ "fuck", "fudge", _chat_filter_anywhere },
	/* (kept as they are: Fukushima, Fukuoka) */
	{ "fuku", NULL, _chat_filter_word_start },
	{ "fuk", "fudge", _chat_filter_word_start },
	{ "fcuk", "fudge", _chat_filter_word_start },
	{ "fck", "fudge", _chat_filter_word_endings },
	{ "wtf", "what the fudge", _chat_filter_word },
	/* (kept as it is: "shit" with its i typed twice) */
	{ "shiit", NULL, _chat_filter_word_start },
	{ "shit", "shiitake", _chat_filter_anywhere },
	{ "biatch", "biscuit", _chat_filter_word_start },
	{ "byatch", "biscuit", _chat_filter_word_start },
	{ "beyotch", "biscuit", _chat_filter_word_start },
	{ "blotch", NULL, _chat_filter_word_start },
	{ "biotch", "biscuit", _chat_filter_word_start },
	{ "bitch", "biscuit", _chat_filter_anywhere },
	{ "asshole", "donut hole", _chat_filter_word_start },
	{ "asshat", "donkey hat", _chat_filter_word_start },
	{ "asswipe", "baby wipe", _chat_filter_word_start },
	{ "dumbass", "dumb donkey", _chat_filter_word },
	{ "fatass", "fat donkey", _chat_filter_word },
	{ "jackass", "jackrabbit", _chat_filter_word },
	/* (kept as they are: the words that begin with ass harmlessly) */
	{ "assai", NULL, _chat_filter_word_start },
	{ "assam", NULL, _chat_filter_word_start },
	{ "assas", NULL, _chat_filter_word_start },
	{ "assau", NULL, _chat_filter_word_start },
	{ "assay", NULL, _chat_filter_word_start },
	{ "assem", NULL, _chat_filter_word_start },
	{ "assen", NULL, _chat_filter_word_start },
	{ "asser", NULL, _chat_filter_word_start },
	{ "assess", NULL, _chat_filter_word_start },
	{ "asset", NULL, _chat_filter_word_start },
	{ "assid", NULL, _chat_filter_word_start },
	{ "assig", NULL, _chat_filter_word_start },
	{ "assim", NULL, _chat_filter_word_start },
	{ "assis", NULL, _chat_filter_word_start },
	{ "assoc", NULL, _chat_filter_word_start },
	{ "asson", NULL, _chat_filter_word_start },
	{ "assor", NULL, _chat_filter_word_start },
	{ "assua", NULL, _chat_filter_word_start },
	{ "assum", NULL, _chat_filter_word_start },
	{ "assur", NULL, _chat_filter_word_start },
	{ "assyr", NULL, _chat_filter_word_start },
	{ "assiz", NULL, _chat_filter_word_start },
	{ "asst", NULL, _chat_filter_word_start },
	{ "assn", NULL, _chat_filter_word_start },
	{ "ass", "donkey", _chat_filter_word_start },
	{ "dickhead", "pickle head", _chat_filter_word_start },
	/* (kept as they are: Dickens, dickey, dicker) */
	{ "dicken", NULL, _chat_filter_word_start },
	{ "dickey", NULL, _chat_filter_word_start },
	{ "dicker", NULL, _chat_filter_word_start },
	{ "dickins", NULL, _chat_filter_word_start },
	{ "dick", "pickle", _chat_filter_anywhere },
	{ "cock", "rooster", _chat_filter_word },
	{ "pussies", "kitty cats", _chat_filter_word },
	/* (kept as they are: pussycat, pussyfoot, pussy willow) */
	{ "pussycat", NULL, _chat_filter_word_start },
	{ "octopussy", NULL, _chat_filter_word_start },
	{ "pussyfoot", NULL, _chat_filter_word_start },
	{ "pussy willow", NULL, _chat_filter_word },
	{ "pussy", "kitty cat", _chat_filter_anywhere },
	{ "whore", "hoarder", _chat_filter_anywhere },
	{ "hoe", "rake", _chat_filter_word },
	{ "thot", "thought", _chat_filter_word },
	{ "slut", "sloth", _chat_filter_anywhere },
	{ "skank", "skunk", _chat_filter_word_start },
	{ "bastard", "custard", _chat_filter_word_start },
	{ "twat", "teapot", _chat_filter_word_start },
	{ "wanker", "walnut", _chat_filter_word_start },
	{ "wank", "walnut", _chat_filter_word_start },
	{ "bollock", "bollard", _chat_filter_word_start },
	{ "douche", "duvet", _chat_filter_word_start },
	{ "prick", "pinecone", _chat_filter_word },
	{ "piss", "fizz", _chat_filter_word_start },
	{ "damnation", NULL, _chat_filter_word_start },
	{ "goddamn", "goshdarn", _chat_filter_word_start },
	{ "goddam", "goshdarn", _chat_filter_word_start },
	{ "damn", "darn", _chat_filter_word_start },
	{ "cum", "cumin", _chat_filter_word },
	{ "jizz", "jazz", _chat_filter_word_start },
	{ "dildo", "didgeridoo", _chat_filter_word_start },
	{ "tittle", NULL, _chat_filter_word_start },
	{ "titties", "tater tots", _chat_filter_word },
	{ "tits", "tater tots", _chat_filter_word },
	{ "mofo", "mojo", _chat_filter_word },
	{ "gfy", "go find yourself", _chat_filter_word },
	/* sexual */
	{ "blowjob", "bubble tea", _chat_filter_word_start },
	{ "gangbang", "gangnam style", _chat_filter_word_start },
	{ "fellat", "fellowship", _chat_filter_word_start },
	{ "felat", "fellowship", _chat_filter_word_start },
	{ "fingerbang", "finger gun", _chat_filter_word_start },
	{ "jism", "jasmine", _chat_filter_word },
	{ "jizm", "jasmine", _chat_filter_word },
	{ "spooge", "sponge", _chat_filter_word_endings },
	{ "camel toe", "camel ride", _chat_filter_word },
	{ "clitor", "clipboard", _chat_filter_word_start },
	{ "molest", "pester", _chat_filter_word_start },
	{ "butthole", "button hole", _chat_filter_word_start },
	{ "buttwipe", "baby wipe", _chat_filter_word_start },
	{ "blow job", "bubble tea", _chat_filter_word },
	{ "handjob", "hand jive", _chat_filter_word_start },
	{ "rimjob", "rim shot", _chat_filter_word_start },
	{ "jerk off", "jerk chicken", _chat_filter_word },
	{ "masturbat", "macaroni", _chat_filter_word_start },
	{ "masterbat", "macaroni", _chat_filter_word_start },
	{ "poorn", NULL, _chat_filter_word_start },
	{ "porn", "popcorn", _chat_filter_word_start },
	{ "horny", "hungry", _chat_filter_word },
	{ "orgasm", "organ solo", _chat_filter_word_start },
	{ "orgies", "origami parties", _chat_filter_word },
	{ "orgy", "origami party", _chat_filter_word },
	{ "sex", "socks", _chat_filter_word_endings },
	{ "annal", NULL, _chat_filter_word_start },
	{ "anal", "annual", _chat_filter_word },
	{ "penis", "peanut", _chat_filter_word_start },
	{ "vagina", "vanilla", _chat_filter_word_start },
	{ "clit", "clip", _chat_filter_word },
	{ "boob", "balloon", _chat_filter_word_endings },
	{ "nutsack", "nut bag", _chat_filter_word_start },
	{ "ballsack", "bean bag", _chat_filter_word_start },
	{ "ball sack", "bean bag", _chat_filter_word },
	{ "queef", "quiche", _chat_filter_word },
	{ "chode", "toad", _chat_filter_word },
	{ "milf", "milk", _chat_filter_word },
	{ "hentai", "anime", _chat_filter_word },
	{ "nudes", "noodles", _chat_filter_word },
	/* other languages' */
	{ "puta", "pato", _chat_filter_word },
	{ "puto", "pato", _chat_filter_word },
	{ "mierda", "merienda", _chat_filter_word },
	{ "pendejo", "pan dulce", _chat_filter_word },
	{ "cabron", "cupcake", _chat_filter_word },
	{ "maricon", "macaron", _chat_filter_word },
	{ "kurwa", "kiwi", _chat_filter_word },
	{ "fanculo", "fan club", _chat_filter_word },
	{ "scheisse", "schnitzel", _chat_filter_word },
	{ "stfu", "shush", _chat_filter_word },
	{ "gtfo", "go touch grass", _chat_filter_word },
};

/* the texts drawn, as streamer mode shows them (chat_filter_shown) */
static struct
{
	boolean valid;
	short length;
	unsigned long hash;
	wchar_t source[CHAT_FILTER_CACHED_SIZE];
	wchar_t shown[CHAT_FILTER_CACHED_SIZE * CHAT_FILTER_GROWTH];
} chat_filter_cache[CHAT_FILTER_CACHED];
static short chat_filter_cache_next;

/* the endings a word may have (_chat_filter_word: the first
CHAT_FILTER_PLURAL_ENDINGS) */
static char const *const chat_filter_endings[] =
{
	"", "s", "es", "z",
	"y", "ie", "ies", "ed", "er", "ers", "in", "ing", "ings", "d", "r", "rs",
};

/* ---------- the filter (pure: tools/harness/tests/chat_filter.c) */

static boolean chat_filter_letter(
	wchar_t character)
{
	return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

/* what a word is made of (a sign that stands for a letter is not: "fag!") */
static boolean chat_filter_word_character(
	wchar_t character)
{
	return chat_filter_letter(character) || (character >= '0' && character <= '9');
}

static wchar_t chat_filter_lower(
	wchar_t character)
{
	return character >= 'A' && character <= 'Z' ? (wchar_t)(character - 'A' + 'a') : character;
}

static boolean chat_filter_in(
	char character,
	char const *set)
{
	for (; *set; set++)
	{
		if (*set == character)
			return TRUE;
	}
	return FALSE;
}

/* whether a character of the text is the word's letter, in either case, or
one that stands for it */
static boolean chat_filter_character_matches(
	wchar_t character,
	char letter)
{
	static char const stand_ins[][2] =
	{
		{ '4', 'a' }, { '@', 'a' }, { '3', 'e' }, { '1', 'i' }, { '!', 'i' }, { '|', 'i' }, { 'l', 'i' },
		{ '1', 'l' }, { '|', 'l' }, { '0', 'o' }, { '$', 's' }, { '5', 's' }, { '7', 't' }, { '+', 't' },
		{ '8', 'b' }, { '9', 'g' }, { '6', 'g' }, { 'q', 'g' }, { 'k', 'c' }, { '(', 'c' }, { '<', 'c' },
		{ 'v', 'u' },
	};
	short index;

	if (chat_filter_lower(character) == (wchar_t)letter)
		return TRUE;
	/* (* for a vowel: "f*ck") */
	if (character == '*' && chat_filter_in(letter, "aeiou"))
		return TRUE;
	for (index = 0; index < (short)NUMBEROF(stand_ins); index++)
	{
		if (chat_filter_lower(character) == (wchar_t)stand_ins[index][0] && letter == stand_ins[index][1])
			return TRUE;
	}
	return FALSE;
}

/* where the word's letters from index match the text from at: the end of
the match, NONE for none. Each letter matches a character, and more of it
("fuuuck"), as many as leave the rest of the word matching (so "ass" needs
two s's); ph matches f ("phuck") */
static short chat_filter_match_from(
	wchar_t const *text,
	short at,
	short length,
	char const *word,
	short index)
{
	short end;
	short last;

	if (!word[index])
		return at;
	if (word[index] == 'f' && at + 1 < length && chat_filter_lower(text[at]) == 'p' &&
		chat_filter_lower(text[at + 1]) == 'h')
	{
		end = chat_filter_match_from(text, (short)(at + 2), length, word, (short)(index + 1));
		if (end != NONE)
			return end;
	}
	if (at >= length || !chat_filter_character_matches(text[at], word[index]))
		return NONE;
	/* (more of the same character, not of another that stands for the
	letter: "tilts" is not "tits") */
	for (last = (short)(at + 1); last < length && chat_filter_lower(text[last]) == chat_filter_lower(text[at]); last++)
		;
	for (; last > at; last--)
	{
		end = chat_filter_match_from(text, last, length, word, (short)(index + 1));
		if (end != NONE)
			return end;
	}
	return NONE;
}

/* how many characters of the text from start the word matches, 0 for none */
static short chat_filter_match(
	wchar_t const *text,
	short start,
	short length,
	char const *word)
{
	short end = chat_filter_match_from(text, start, length, word, 0);

	return end == NONE ? 0 : (short)(end - start);
}

/* what may stand between the letters of a word spelled out: neither a
letter nor a digit, nor a sign that stands for a letter */
static boolean chat_filter_separator(
	wchar_t character)
{
	return !chat_filter_word_character(character) && !chat_filter_in((char)character, "@$!|+*(<") &&
		character < 128;
}

/* how many characters of the text from start are the word spelled out, a
character for each of its letters with separators between them ("n i g g
e r", "c.u.n.t"), and nothing of a word after it; 0 for none (a phrase is
never spelled out) */
static short chat_filter_match_spelled(
	wchar_t const *text,
	short start,
	short length,
	char const *word)
{
	short at = start;
	short index;

	for (index = 0; word[index]; index++)
	{
		if (word[index] == ' ')
			return 0;
		if (index > 0)
		{
			short gap = at;

			while (at < length && chat_filter_separator(text[at]))
				at++;
			if (at == gap)
				return 0;
		}
		if (at >= length || !chat_filter_character_matches(text[at], word[index]))
			return 0;
		at++;
	}
	if (at < length && chat_filter_word_character(text[at]))
		return 0;
	return (short)(at - start);
}

/* whether the rest of a word is an ending it may have (all: the longer
list, else a plural's), each of its letters typed once or more ("zzz") */
static boolean chat_filter_ending(
	wchar_t const *rest,
	short length,
	boolean all)
{
	short count = all ? (short)NUMBEROF(chat_filter_endings) : CHAT_FILTER_PLURAL_ENDINGS;
	short ending;

	for (ending = 0; ending < count; ending++)
	{
		char const *text = chat_filter_endings[ending];
		short at = 0;
		short index;

		for (index = 0; text[index]; index++)
		{
			if (at >= length || chat_filter_lower(rest[at]) != (wchar_t)text[index])
				break;
			at++;
			while (text[index + 1] != text[index] && at < length && chat_filter_lower(rest[at]) == (wchar_t)text[index])
				at++;
		}
		if (!text[index] && at == length)
			return TRUE;
	}
	return FALSE;
}

static void chat_filter_put(
	wchar_t *destination,
	short size,
	short *length,
	wchar_t character)
{
	if (*length < size - 1)
		destination[(*length)++] = character;
}

/* the replacement written for the matched text: in its case (all capitals,
or the first), and fitted to what follows it in its word (fudge + ing:
fudging; biscuit + es: biscuits; flurry + s: flurries); how many of the
characters that follow it the replacement takes in (es's e) */
static short chat_filter_replace(
	wchar_t *destination,
	short size,
	short *length,
	char const *replacement,
	wchar_t const *matched,
	short matched_length,
	wchar_t const *rest,
	short rest_length)
{
	short replacement_length = 0;
	short written;
	short letters = 0;
	short capitals = 0;
	short taken = 0;
	short index;
	boolean all_capitals;
	boolean first_capital = matched[0] >= 'A' && matched[0] <= 'Z';
	wchar_t next = rest_length > 0 ? chat_filter_lower(rest[0]) : 0;
	char last;
	char before_last;
	char const *ending = "";

	while (replacement[replacement_length])
		replacement_length++;
	written = replacement_length;
	last = replacement[replacement_length - 1];
	before_last = replacement_length > 1 ? replacement[replacement_length - 2] : 0;
	for (index = 0; index < matched_length; index++)
	{
		if (chat_filter_letter(matched[index]))
		{
			letters++;
			if (matched[index] >= 'A' && matched[index] <= 'Z')
				capitals++;
		}
	}
	all_capitals = letters >= 2 && capitals == letters;
	if (next == 'e' && rest_length >= 2 && chat_filter_lower(rest[1]) == 's' &&
		(rest_length == 2 || !chat_filter_letter(rest[2])) && !chat_filter_in(last, "sxzh"))
	{
		taken = 1;
	}
	else if (next == 's' && (rest_length == 1 || !chat_filter_letter(rest[1])) && last == 'y' &&
		!chat_filter_in(before_last, "aeiou"))
	{
		written--;
		ending = "ie";
	}
	else if (last == 'e' && (next == 'e' || next == 'i' || next == 'y'))
	{
		written--;
	}
	for (index = 0; index < written; index++)
	{
		wchar_t character = (wchar_t)replacement[index];

		if (all_capitals || (first_capital && index == 0))
			character = character >= 'a' && character <= 'z' ? (wchar_t)(character - 'a' + 'A') : character;
		chat_filter_put(destination, size, length, character);
	}
	for (index = 0; ending[index]; index++)
		chat_filter_put(destination, size, length, all_capitals ? (wchar_t)(ending[index] - 'a' + 'A') : (wchar_t)ending[index]);
	return taken;
}

short chat_filter_text(
	wchar_t *destination,
	short size,
	wchar_t const *source)
{
	short length = 0;
	short source_length = 0;
	short at = 0;

	if (size <= 0)
		return 0;
	while (source[source_length] && source_length < SHORT_MAX - 1)
		source_length++;
	while (at < source_length)
	{
		boolean word_start = at == 0 || !chat_filter_word_character(source[at - 1]);
		short matched = 0;
		short entry;

		for (entry = 0; entry < (short)NUMBEROF(chat_filter_words); entry++)
		{
			struct chat_filter_word const *word = &chat_filter_words[entry];
			short word_end;

			if (word->match != _chat_filter_anywhere && !word_start)
				continue;
			matched = chat_filter_match(source, at, source_length, word->word);
			if (matched)
			{
				for (word_end = (short)(at + matched);
					word_end < source_length && chat_filter_word_character(source[word_end]); word_end++)
				{
				}
				if ((word->match == _chat_filter_word || word->match == _chat_filter_word_endings) &&
					!chat_filter_ending(source + at + matched, (short)(word_end - at - matched),
						word->match == _chat_filter_word_endings))
				{
					matched = 0;
				}
			}
			/* (spelled out: the whole of it replaced) */
			if (!matched && word_start)
				matched = chat_filter_match_spelled(source, at, source_length, word->word);
			if (!matched)
				continue;
			if (!word->replacement)
			{
				short index;

				for (index = 0; index < matched; index++)
					chat_filter_put(destination, size, &length, source[at + index]);
				at += matched;
				break;
			}
			matched += chat_filter_replace(destination, size, &length, word->replacement, source + at, matched,
				source + at + matched, (short)(source_length - at - matched));
			at += matched;
			break;
		}
		if (!matched)
			chat_filter_put(destination, size, &length, source[at++]);
	}
	destination[length] = 0;
	return length;
}

/* a text as the game draws it: the runs between its format codes (a | and the
character after it: "|n", "|t", draw_string.c's) each filtered on its own, the
codes kept as they are, so that none joins the words beside it */
static short chat_filter_formatted(
	wchar_t *destination,
	short size,
	wchar_t const *source)
{
	wchar_t run[CHAT_FILTER_RUN_SIZE];
	short length = 0;
	long index = 0;

	if (size <= 0)
		return 0;
	while (source[index] && length < size - 1)
	{
		short run_length = 0;

		while (source[index] && source[index] != '|' && run_length < CHAT_FILTER_RUN_SIZE - 1)
			run[run_length++] = source[index++];
		run[run_length] = 0;
		if (run_length)
			length = (short)(length + chat_filter_text(destination + length, (short)(size - length), run));
		if (source[index] == '|')
		{
			chat_filter_put(destination, size, &length, source[index++]);
			if (source[index])
				chat_filter_put(destination, size, &length, source[index++]);
		}
	}
	destination[length] = 0;
	return length;
}

/* the text as streamer mode shows it (the string itself when it is off), the
same strings drawn each frame found again in a cache; a text too long for the
cache's entries is shown as it is rather than cut short */
static wchar_t const *chat_filter_cached(
	wchar_t const *string)
{
	unsigned long hash = 2166136261UL;
	long length;
	short slot;
	short index;

	for (length = 0; string[length]; length++)
	{
		if (length >= CHAT_FILTER_CACHED_SIZE - 1)
			return string;
		hash = (hash ^ (unsigned long)string[length]) * 16777619UL;
	}
	for (slot = 0; slot < CHAT_FILTER_CACHED; slot++)
	{
		if (!chat_filter_cache[slot].valid || chat_filter_cache[slot].hash != hash ||
			chat_filter_cache[slot].length != length)
		{
			continue;
		}
		for (index = 0; index < length && chat_filter_cache[slot].source[index] == string[index]; index++)
			;
		if (index == length)
			return chat_filter_cache[slot].shown;
	}
	slot = chat_filter_cache_next;
	chat_filter_cache_next = (short)((chat_filter_cache_next + 1) % CHAT_FILTER_CACHED);
	for (index = 0; index <= length; index++)
		chat_filter_cache[slot].source[index] = string[index];
	chat_filter_cache[slot].hash = hash;
	chat_filter_cache[slot].length = (short)length;
	chat_filter_cache[slot].valid = TRUE;
	chat_filter_formatted(chat_filter_cache[slot].shown, (short)NUMBEROF(chat_filter_cache[slot].shown), string);
	return chat_filter_cache[slot].shown;
}

wchar_t const *chat_filter_shown(
	wchar_t const *string)
{
	if (!string || !chat_filter_enabled())
		return string;
	return chat_filter_cached(string);
}

char const *chat_filter_shown_ascii(
	char const *string)
{
	static char shown[CHAT_FILTER_CACHED_SIZE * CHAT_FILTER_GROWTH];
	wchar_t wide[CHAT_FILTER_CACHED_SIZE];
	wchar_t const *filtered;
	short index;

	if (!string || !chat_filter_enabled())
		return string;
	for (index = 0; string[index]; index++)
	{
		if (index >= CHAT_FILTER_CACHED_SIZE - 1)
			return string;
		wide[index] = (wchar_t)(unsigned char)string[index];
	}
	wide[index] = 0;
	filtered = chat_filter_cached(wide);
	for (index = 0; filtered[index] && index < (short)sizeof(shown) - 1; index++)
		shown[index] = (char)filtered[index];
	shown[index] = 0;
	return shown;
}

/* ---------- the setting */

boolean chat_filter_enabled(
	void)
{
	static boolean enabled;
	static unsigned long read_at = (unsigned long)-1;

	/* (read again when Settings changes it) */
	if (read_at != config_changes())
	{
		read_at = config_changes();
		enabled = config_boolean("game.streamer_mode") != 0;
	}
	return enabled;
}
