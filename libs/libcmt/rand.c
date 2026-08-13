/*
RAND.C

symbols in this file:
	_srand (.text, 0x0D)
	_rand (.text, 0x22)
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
};

/* ---------- constants */

enum
{
	_random_multiplier = 214013,
	_random_increment = 2531011,
	_random_result_shift = 16,
	_random_result_mask = 0x7FFF
};

/* ---------- external code */

struct thread_data* __cdecl _getptd(void);

/* ---------- public code */

void __cdecl srand(
	unsigned int seed)
{
	_getptd()->random_seed = seed;
	return;
}

int __cdecl rand(void)
{
	struct thread_data* thread_data = _getptd();

	thread_data->random_seed = thread_data->random_seed * _random_multiplier + _random_increment;
	return (thread_data->random_seed >> _random_result_shift) & _random_result_mask;
}
