/*
STRTOK.C

symbols in this file:
	_strtok (.text, 0xa5)
*/

/* ---------- types */

struct thread_data
{
	unsigned long thread_id;
	unsigned long thread_handle;
	int error_number;
	unsigned long dos_error_number;
	unsigned int floating_point_data_segment;
	unsigned long random_seed;
	char *token;
	unsigned short *wide_token;
};

/* ---------- external code */

struct thread_data * __cdecl _getptd(
	void);

/* ---------- public code */

char * __cdecl strtok(
	char *string,
	char const *control)
{
	unsigned char *scan;
	unsigned char const *control_character = (unsigned char const *)control;
	unsigned char map[32];
	int count;
	struct thread_data *thread_data = _getptd();

	for (count = 0; count < 32; count++)
		map[count] = 0;

	do
	{
		map[*control_character >> 3] |= 1 << (*control_character & 7);
	}
	while (*control_character++);

	if (string)
		scan = (unsigned char *)string;
	else
		scan = (unsigned char *)thread_data->token;

	while ((map[*scan >> 3] & (1 << (*scan & 7))) && *scan)
		scan++;

	string = (char *)scan;

	for (; *scan; scan++)
	{
		if (map[*scan >> 3] & (1 << (*scan & 7)))
		{
			*scan++ = 0;
			break;
		}
	}

	thread_data->token = (char *)scan;

	return string == (char *)scan ? 0 : string;
}
