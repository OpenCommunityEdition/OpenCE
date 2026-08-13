/*
STRERROR.C

symbols in this file:
	_strerror (.text, 0x51)
	?errmsg_backup@?1??strerror@@9@9 (.bss, 0x28)
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
	unsigned char *multibyte_token;
	char *error_message;
};

/* ---------- constants */

enum
{
	_error_message_allocation_size = 134,
	_error_message_backup_size = 40
};

/* ---------- public data */

char bss_004e3eb0[_error_message_backup_size] = { 0 };

/* ---------- external data */

extern char const *_sys_errlist[];
extern int _sys_nerr;

/* ---------- external code */

struct thread_data * __cdecl _getptd(
	void);

void * __cdecl malloc(
	unsigned long size);

/* ---------- public code */

char * __cdecl strerror(
	int error_number)
{
	struct thread_data *thread_data = _getptd();
	char *error_message;
	char const *source;
	char *destination;
	char character;

	if (!thread_data->error_message &&
		!(thread_data->error_message = (char *)malloc(_error_message_allocation_size)))
		error_message = bss_004e3eb0;
	else
		error_message = thread_data->error_message;

	source = _sys_errlist[
		error_number < 0 || error_number >= _sys_nerr
			? _sys_nerr
			: error_number];
	destination = error_message;
	do
	{
		character = *source++;
		*destination++ = character;
	}
	while (character);

	return error_message;
}
