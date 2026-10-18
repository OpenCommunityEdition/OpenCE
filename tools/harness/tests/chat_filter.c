/*
CHAT_FILTER.C (test)

Streamer mode's word filter (port/linux/game/chat_filter.c): what it
replaces, in which spellings, what it leaves alone, the case and endings
of what it writes, and that it writes no further than its buffer.
test_chat_filter.py takes the list, its constants and the code
(under_test.inc).
*/

#include "harness.h"

#include <wchar.h>

#define SHORT_MAX 32767
#define NUMBEROF(array) ((long)(sizeof(array) / sizeof((array)[0])))

/* streamer mode, as the setting says */
static boolean enabled = TRUE;
#define chat_filter_enabled() enabled
static short chat_filter_cache_next;

#include "under_test.inc"

/* a text in ASCII as the game would draw it */
static char const *shown(
	char const *text)
{
	return chat_filter_shown_ascii(text);
}

#define EXPECT_SHOWN(text, expected) \
	CHECK(!strcmp(shown(text), expected), "\"%s\" drawn as \"%s\", not \"%s\"", text, shown(text), expected)

/* the text filtered, in ASCII */
static char const *filtered(
	char const *text)
{
	static char result[256];
	wchar_t source[256];
	wchar_t destination[256];
	short index;

	for (index = 0; text[index]; index++)
		source[index] = (wchar_t)(unsigned char)text[index];
	source[index] = 0;
	chat_filter_text(destination, (short)NUMBEROF(destination), source);
	for (index = 0; destination[index]; index++)
		result[index] = (char)destination[index];
	result[index] = 0;
	return result;
}

#define EXPECT(text, expected) \
	CHECK(!strcmp(filtered(text), expected), "\"%s\" shows as \"%s\", not \"%s\"", text, filtered(text), expected)

int main(int argc, char **argv)
{
	char const *case_name = argc > 1 ? argv[1] : "";

	/* the words replaced, as in the request: faggot, cunt */
	CASE("replaces")
	{
		EXPECT("faggot", "flurry");
		EXPECT("you cunt", "you chop stick");
		EXPECT("what the fuck", "what the fudge");
		EXPECT("shit", "shiitake");
		EXPECT("kys", "keep yourself safe");
		EXPECT("go kill yourself", "go treat yourself");
	}

	/* in the spellings that dodge a plain list */
	CASE("spellings")
	{
		EXPECT("FAGGOT", "FLURRY");
		EXPECT("f4gg0t", "flurry");
		EXPECT("phaggot", "flurry");
		EXPECT("fuuuuck", "fudge");
		EXPECT("fvck", "fudge");
		EXPECT("f*ck", "fudge");
		EXPECT("$hit", "shiitake");
		EXPECT("kunt", "chop stick");
		EXPECT("@ss", "donkey");
		EXPECT("kill   yourself", "treat yourself");
	}

	/* the dodges asked about: digits for letters, endings, spelled out */
	CASE("dodges")
	{
		EXPECT("n1gger", "nugget");
		EXPECT("n1gg3rz", "nuggetz");
		EXPECT("c00n", "cocoon");
		EXPECT("lynch dat melon monkey", "high five dat melon baller");
		EXPECT("niqqa", "ninja");
		EXPECT("nlgga", "ninja");
		EXPECT("n*gga", "ninja");
		EXPECT("niggggaaa", "ninja");
		EXPECT("niggazzz", "ninjazzz");
		EXPECT("n i g g e r", "nugget");
		EXPECT("N.I.G.G.E.R", "NUGGET");
		EXPECT("c-u-n-t", "chop stick");
		EXPECT("fukin", "fudgin");
	}

	/* the harmless words that hold one */
	CASE("leaves-alone")
	{
		EXPECT("Scunthorpe", "Scunthorpe");
		EXPECT("spicy food", "spicy food");
		EXPECT("raccoon", "raccoon");
		EXPECT("cocky", "cocky");
		EXPECT("assassin class pass", "assassin class pass");
		EXPECT("as", "as");
		EXPECT("cumulative", "cumulative");
		EXPECT("fire retardant", "fire retardant");
		EXPECT("therapist", "therapist");
		EXPECT("shiitake", "shiitake");
		EXPECT("hello, nice shot!", "hello, nice shot!");
		EXPECT("snigger at Niger, niggle, night", "snigger at Niger, niggle, night");
		EXPECT("niggardly", "niggardly");
		EXPECT("it tilts", "it tilts");
		EXPECT("Hitchcock peacock cocktail", "Hitchcock peacock cocktail");
		EXPECT("homosexual Japan tardy pedometer", "homosexual Japan tardy pedometer");
		EXPECT("this is hit or miss", "this is hit or miss");
		EXPECT("a s b", "a s b");
		/* (found running an English dictionary through the list) */
		EXPECT("FAQ rapper Shiite Ashkenazi snazzier blotch annals woops", "FAQ rapper Shiite Ashkenazi snazzier blotch annals woops");
		EXPECT("cocked Assyrian spaces spices Damnation", "cocked Assyrian spaces spices Damnation");
		EXPECT("pussycat Dickens Fukushima Octopussy assist asst", "pussycat Dickens Fukushima Octopussy assist asst");
	}

	/* words of the list inside others, run together */
	CASE("compounds")
	{
		EXPECT("cumslut", "cumsloth");
		EXPECT("sonofabitch", "sonofabiscuit");
		EXPECT("thundercunt", "thunderchop stick");
		EXPECT("asshat", "donkey hat");
		EXPECT("assbandit", "donkeybandit");
		EXPECT("neonazi", "neo nacho");
		EXPECT("suckmydick", "suckmypickle");
		EXPECT("raped", "graped");
	}

	/* the case of what it replaces */
	CASE("case")
	{
		EXPECT("Fuck", "Fudge");
		EXPECT("FUCK OFF", "FUDGE OFF");
		EXPECT("Cunt", "Chop stick");
	}

	/* the endings, kept and fitted */
	CASE("endings")
	{
		EXPECT("fucking", "fudging");
		EXPECT("motherfucker", "motherfudger");
		EXPECT("bitches", "biscuits");
		EXPECT("faggots", "flurries");
		EXPECT("cunts", "chop sticks");
		EXPECT("retarded", "rutabagaed");
		EXPECT("asses", "donkeys");
		EXPECT("spics", "sporks");
		EXPECT("fag!", "flapjack!");
	}

	/* a text drawn: its format codes (| and the character after) kept, and
	never joined to the words beside them */
	CASE("formatted")
	{
		EXPECT_SHOWN("Player 2|tits a trap", "Player 2|tits a trap");
		EXPECT_SHOWN("|nfuck you|tcunt", "|nfudge you|tchop stick");
		EXPECT_SHOWN("fu|nck", "fu|nck");
		EXPECT_SHOWN("ends with a bar|", "ends with a bar|");
	}

	/* every text drawn: the same text again from the cache, another text
	not mistaken for it; as it is with streamer mode off, or too long */
	CASE("shown")
	{
		char long_text[400];
		wchar_t wide[8] = L"fuck";
		wchar_t long_wide[400];
		short index;

		EXPECT_SHOWN("fuck", "fudge");
		EXPECT_SHOWN("abcd", "abcd");
		EXPECT_SHOWN("fuck", "fudge");
		CHECK(chat_filter_shown(wide)[1] == 'u' && chat_filter_shown(wide)[3] == 'g', "the wide text not replaced");
		memset(long_text, 'x', sizeof(long_text) - 6);
		memcpy(long_text + sizeof(long_text) - 6, "fuck", 5);
		CHECK(shown(long_text) == long_text, "a text too long for the cache not drawn as it is");
		for (index = 0; index < 399; index++)
			long_wide[index] = (wchar_t)long_text[index];
		long_wide[399] = 0;
		CHECK(chat_filter_shown(long_wide) == long_wide, "a wide text too long for the cache not drawn as it is");
		enabled = FALSE;
		CHECK(shown("fuck") != NULL && !strcmp(shown("fuck"), "fuck"), "replaced with streamer mode off");
		CHECK(chat_filter_shown(wide) == wide, "not the string itself with streamer mode off");
	}

	/* what replaces a word holds none (the chat's text and the drawn text are
	both filtered) */
	CASE("idempotent")
	{
		long index;

		for (index = 0; index < NUMBEROF(chat_filter_words); index++)
		{
			wchar_t source[64];
			wchar_t destination[256];
			short length;

			if (!chat_filter_words[index].replacement)
				continue;
			for (length = 0; chat_filter_words[index].replacement[length]; length++)
				source[length] = (wchar_t)chat_filter_words[index].replacement[length];
			source[length] = 0;
			chat_filter_text(destination, (short)NUMBEROF(destination), source);
			CHECK(!wcscmp(destination, source), "\"%s\" (for \"%s\") is itself replaced",
				chat_filter_words[index].replacement, chat_filter_words[index].word);
		}
	}

	/* never past the buffer, and always ended */
	CASE("bounds")
	{
		wchar_t source[64];
		wchar_t destination[8];
		short index;
		short length;

		for (index = 0; index < 40; index++)
			source[index] = L"fuck "[index % 5];
		source[40] = 0;
		for (index = 0; index < 8; index++)
			destination[index] = 0x7777;
		length = chat_filter_text(destination, 6, source);
		CHECK(length == 5 && destination[5] == 0, "length %d", length);
		CHECK(destination[6] == 0x7777 && destination[7] == 0x7777, "written past its size");
		CHECK(chat_filter_text(destination, 1, source) == 0 && destination[0] == 0, "a size of 1 not empty");
	}

	return 0;
}
