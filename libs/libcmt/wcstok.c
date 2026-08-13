/*
WCSTOK.C

symbols in this file:
	_wcstok (.text, 0xA5)
*/

/* ---------- types */

typedef unsigned short wchar_t;

struct thread_data
{
	unsigned long thread_id;
	unsigned long thread_handle;
	int error_number;
	unsigned long dos_error_number;
	unsigned int floating_point_data_segment;
	unsigned long random_seed;
	char *token;
	wchar_t *wide_token;
};

/* ---------- external code */

struct thread_data * __cdecl _getptd(
	void);

/* ---------- public code */

wchar_t * __cdecl wcstok(
	wchar_t *string,
	wchar_t const *control)
{
	struct thread_data *thread_data = _getptd();
	wchar_t const *control_character;
	wchar_t *token;

	if (!string)
		string = thread_data->wide_token;

	while (*string)
	{
		control_character = control;

		while (*control_character && *control_character != *string)
			control_character++;

		if (!*control_character)
			break;

		string++;
	}

	token = string;

	while (*string)
	{
		control_character = control;

		while (*control_character && *control_character != *string)
			control_character++;

		if (*control_character)
		{
			*string++ = 0;
			break;
		}

		string++;
	}

	thread_data->wide_token = string;

	return token == string ? 0 : token;
}
