#define NULL ((void *)0)
struct transport_endpoint;

int __cdecl c_aB_Ab_e(void const *a, void const *b)
{
	struct transport_endpoint *endpoint_a = *(struct transport_endpoint *const *)a;
	struct transport_endpoint *endpoint_b = *(struct transport_endpoint *const *)b;

	if (!endpoint_a && endpoint_b)
	{
		return 1;
	}
	else if (endpoint_a && !endpoint_b)
	{
		return -1;
	}
	return 0;
}

int __cdecl c_aB_Ab_n(void const *a, void const *b)
{
	struct transport_endpoint *endpoint_a = *(struct transport_endpoint *const *)a;
	struct transport_endpoint *endpoint_b = *(struct transport_endpoint *const *)b;

	if (!endpoint_a && endpoint_b)
	{
		return 1;
	}
	if (endpoint_a && !endpoint_b)
	{
		return -1;
	}
	return 0;
}

int __cdecl c_aB_bA_e(void const *a, void const *b)
{
	struct transport_endpoint *endpoint_a = *(struct transport_endpoint *const *)a;
	struct transport_endpoint *endpoint_b = *(struct transport_endpoint *const *)b;

	if (!endpoint_a && endpoint_b)
	{
		return 1;
	}
	else if (!endpoint_b && endpoint_a)
	{
		return -1;
	}
	return 0;
}

int __cdecl c_aB_bA_n(void const *a, void const *b)
{
	struct transport_endpoint *endpoint_a = *(struct transport_endpoint *const *)a;
	struct transport_endpoint *endpoint_b = *(struct transport_endpoint *const *)b;

	if (!endpoint_a && endpoint_b)
	{
		return 1;
	}
	if (!endpoint_b && endpoint_a)
	{
		return -1;
	}
	return 0;
}

int __cdecl c_Ba_Ab_e(void const *a, void const *b)
{
	struct transport_endpoint *endpoint_a = *(struct transport_endpoint *const *)a;
	struct transport_endpoint *endpoint_b = *(struct transport_endpoint *const *)b;

	if (endpoint_b && !endpoint_a)
	{
		return 1;
	}
	else if (endpoint_a && !endpoint_b)
	{
		return -1;
	}
	return 0;
}

int __cdecl c_Ba_Ab_n(void const *a, void const *b)
{
	struct transport_endpoint *endpoint_a = *(struct transport_endpoint *const *)a;
	struct transport_endpoint *endpoint_b = *(struct transport_endpoint *const *)b;

	if (endpoint_b && !endpoint_a)
	{
		return 1;
	}
	if (endpoint_a && !endpoint_b)
	{
		return -1;
	}
	return 0;
}

int __cdecl c_Ba_bA_e(void const *a, void const *b)
{
	struct transport_endpoint *endpoint_a = *(struct transport_endpoint *const *)a;
	struct transport_endpoint *endpoint_b = *(struct transport_endpoint *const *)b;

	if (endpoint_b && !endpoint_a)
	{
		return 1;
	}
	else if (!endpoint_b && endpoint_a)
	{
		return -1;
	}
	return 0;
}

int __cdecl c_Ba_bA_n(void const *a, void const *b)
{
	struct transport_endpoint *endpoint_a = *(struct transport_endpoint *const *)a;
	struct transport_endpoint *endpoint_b = *(struct transport_endpoint *const *)b;

	if (endpoint_b && !endpoint_a)
	{
		return 1;
	}
	if (!endpoint_b && endpoint_a)
	{
		return -1;
	}
	return 0;
}
