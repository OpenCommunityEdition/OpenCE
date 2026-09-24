#define NULL ((void *)0)
struct transport_endpoint;
#define LOADS \
	struct transport_endpoint *endpoint_a = *(struct transport_endpoint *const *)a; \
	struct transport_endpoint *endpoint_b = *(struct transport_endpoint *const *)b;

int __cdecl c_both(void const *a, void const *b)
{
	LOADS
	if (endpoint_a && endpoint_b) return 0;
	if (endpoint_a) return -1;
	if (endpoint_b) return 1;
	return 0;
}
int __cdecl c_none(void const *a, void const *b)
{
	LOADS
	if (!endpoint_a && !endpoint_b) return 0;
	if (!endpoint_a) return 1;
	if (!endpoint_b) return -1;
	return 0;
}
int __cdecl c_tern2(void const *a, void const *b)
{
	LOADS
	if (endpoint_a) return endpoint_b ? 0 : -1;
	else return endpoint_b ? 1 : 0;
}
int __cdecl c_tern3(void const *a, void const *b)
{
	LOADS
	if (!endpoint_a) return endpoint_b ? 1 : 0;
	else return endpoint_b ? 0 : -1;
}
int __cdecl c_res_tern(void const *a, void const *b)
{
	LOADS
	int result;
	if (endpoint_a == NULL) result = (endpoint_b != NULL) ? 1 : 0;
	else result = (endpoint_b == NULL) ? -1 : 0;
	return result;
}
int __cdecl c_nest_else0(void const *a, void const *b)
{
	LOADS
	if (!endpoint_a)
	{
		if (endpoint_b) return 1;
		else return 0;
	}
	else
	{
		if (!endpoint_b) return -1;
		else return 0;
	}
}
int __cdecl c_eqeq(void const *a, void const *b)
{
	LOADS
	if ((endpoint_a == NULL) == (endpoint_b == NULL)) return 0;
	if (endpoint_a == NULL) return 1;
	return -1;
}
int __cdecl c_andnot(void const *a, void const *b)
{
	LOADS
	if (endpoint_a == NULL && endpoint_b != NULL) return 1;
	else if (endpoint_b == NULL && endpoint_a != NULL) return -1;
	else return 0;
}
