/*
WCHTODIG.C

symbols in this file:
	__wchartodigit (.text, 0x176)
*/

#include <wchar.h>

/* ---------- constants */

enum
{
	unicode_digit_count = 10,
	unicode_ascii_digit_zero = 0x0030,
	unicode_arabic_indic_digit_zero = 0x0660,
	unicode_extended_arabic_indic_digit_zero = 0x06F0,
	unicode_devanagari_digit_zero = 0x0966,
	unicode_bengali_digit_zero = 0x09E6,
	unicode_gurmukhi_digit_zero = 0x0A66,
	unicode_gujarati_digit_zero = 0x0AE6,
	unicode_oriya_digit_zero = 0x0B66,
	unicode_telugu_digit_zero = 0x0C66,
	unicode_kannada_digit_zero = 0x0CE6,
	unicode_malayalam_digit_zero = 0x0D66,
	unicode_thai_digit_zero = 0x0E50,
	unicode_lao_digit_zero = 0x0ED0,
	unicode_tibetan_digit_zero = 0x0F20,
	unicode_myanmar_digit_zero = 0x1040,
	unicode_khmer_digit_zero = 0x17E0,
	unicode_mongolian_digit_zero = 0x1810,
	unicode_fullwidth_digit_zero = 0xFF10
};

#define TEST_UNICODE_DIGIT_RANGE(character, zero) \
	if ((character) < (zero)) \
		return -1; \
	if ((character) < (zero) + unicode_digit_count) \
	{ \
		return (character) - (zero); \
	}

/* ---------- public code */

int __cdecl _wchartodigit(
	wchar_t character)
{
	TEST_UNICODE_DIGIT_RANGE(character, unicode_ascii_digit_zero);

	if (character < unicode_fullwidth_digit_zero)
	{
		TEST_UNICODE_DIGIT_RANGE(character, unicode_arabic_indic_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_extended_arabic_indic_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_devanagari_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_bengali_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_gurmukhi_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_gujarati_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_oriya_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_telugu_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_kannada_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_malayalam_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_thai_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_lao_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_tibetan_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_myanmar_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_khmer_digit_zero);
		TEST_UNICODE_DIGIT_RANGE(character, unicode_mongolian_digit_zero);

		return -1;
	}

	if (character < unicode_fullwidth_digit_zero + unicode_digit_count)
		return character - unicode_fullwidth_digit_zero;

	return -1;
}

#undef TEST_UNICODE_DIGIT_RANGE
