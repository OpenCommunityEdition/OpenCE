/*
THUNK_TEST.H

The imports of port/windows/tests/thunk_test.c, as a guest would declare
them: tools/windows_host_thunks.py makes their thunks from these
declarations (tools/test_windows_host_thunks.py).
*/

long long host_test_integers(unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5,
	unsigned int a6, unsigned int a7, unsigned int a8, long long a9, unsigned int a10, unsigned int a11,
	unsigned int a12, unsigned int a13, unsigned int a14, unsigned int a15);
double host_test_mixed(int a, float b, int c, double d, int e, float f, int g, int h, int i, float j);
double host_test_floats(double x1, double x2, double x3, double x4, double x5, double x6, double x7, double x8,
	double x9, float x10);
float host_test_pointer(const char *text, float scale);
int host_test_nested(int depth);
void host_test_raise(void);
int host_test_unwind(void);
