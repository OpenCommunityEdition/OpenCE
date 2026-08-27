/* Which C shapes make VC7 pass a float argument through the FPU?

January, in the hs boolean wrappers and code_000b3f50:
    fld  dword ptr [eax+4]      ; load the float
    push ecx                    ; reserve 4 bytes
    fstp dword ptr [esp]        ; fill them
We emit `mov edx,[eax+4]; push edx` instead. `volatile` produces January's form
but is forbidden. Find a legal shape that does the same.
*/

typedef float real;
typedef unsigned short word;
typedef unsigned char boolean;

union arg
{
	long long_value;
	real real_value;
	short short_value;
	word unsigned_short_value;
};

struct s_two
{
	long value0;
	real value1;
};

extern boolean sink(long a, real b);
extern void *evaluate(short fi, long ti, boolean init);

/* v01: the plain form - what we emit today */
boolean v01(union arg const *a)
{
	return sink(a[0].long_value, a[1].real_value);
}

/* v02: through a struct rather than a union */
boolean v02(struct s_two const *a)
{
	return sink(a->value0, a->value1);
}

/* v03: named local for the real (adds a slot - known wrong, kept as control) */
boolean v03(union arg const *a)
{
	real b = a[1].real_value;
	return sink(a[0].long_value, b);
}

/* v04: read through a pointer-to-real */
boolean v04(union arg const *a)
{
	real const *p = &a[1].real_value;
	return sink(a[0].long_value, *p);
}

/* v05: pointer-to-real, no named pointer, deref of address-of */
boolean v05(union arg const *a)
{
	return sink(a[0].long_value, *&a[1].real_value);
}

/* v06: index the union as a real array via a real pointer */
boolean v06(real const *r, union arg const *a)
{
	return sink(a[0].long_value, r[1]);
}

/* v07: the real arrives already as a parameter */
boolean v07(union arg const *a, real b)
{
	return sink(a[0].long_value, b);
}

/* v08: double parameter in the callee's slot - control, expect qword */
extern boolean sink_d(long a, double b);
boolean v08(union arg const *a)
{
	return sink_d(a[0].long_value, a[1].real_value);
}

/* v09: the union member accessed through a second union variable */
boolean v09(union arg const *a)
{
	union arg t;
	t = a[1];
	return sink(a[0].long_value, t.real_value);
}

/* v10: struct copy then member */
boolean v10(struct s_two const *a)
{
	struct s_two t;
	t = *a;
	return sink(t.value0, t.value1);
}

/* v11: the January calling context - evaluate, test, then call */
boolean v11(short fi, long ti, boolean init)
{
	union arg const *a;
	boolean r = 0;

	a = (union arg const *)evaluate(fi, ti, init);
	if (a)
		r = sink(a[0].long_value, a[1].real_value);
	return r;
}

/* v12: same, but the real read through a real* alias of the block */
boolean v12(short fi, long ti, boolean init)
{
	union arg const *a;
	real const *r;
	boolean res = 0;

	a = (union arg const *)evaluate(fi, ti, init);
	r = (real const *)a;
	if (a)
		res = sink(a[0].long_value, r[1]);
	return res;
}

/* v13: const real local */
boolean v13(union arg const *a)
{
	real const b = a[1].real_value;
	return sink(a[0].long_value, b);
}

/* v14: real member of a const struct read twice (defeats the single-copy fold) */
boolean v14(union arg const *a)
{
	return sink(a[0].long_value, a[1].real_value) && a[1].real_value != 0.f;
}

/* --- second round: make the read an FPU-valued expression --- */

/* v15: a small accessor helper returning real (natural, readable C) */
static real arg_real(union arg const *a, long i)
{
	return a[i].real_value;
}
boolean v15(union arg const *a)
{
	return sink(a[0].long_value, arg_real(a, 1));
}

/* v16: helper taking the element */
static real elem_real(union arg const *e)
{
	return e->real_value;
}
boolean v16(union arg const *a)
{
	return sink(a[0].long_value, elem_real(&a[1]));
}

/* v17: unary plus */
boolean v17(union arg const *a)
{
	return sink(a[0].long_value, +a[1].real_value);
}

/* v18: explicit same-type cast */
boolean v18(union arg const *a)
{
	return sink(a[0].long_value, (real)a[1].real_value);
}

/* v19: through a double intermediate (real -> double -> real) */
boolean v19(union arg const *a)
{
	double d = a[1].real_value;
	return sink(a[0].long_value, (real)d);
}

/* v20: the callee prototyped to take double, argument a real local */
boolean v20(union arg const *a)
{
	real b = a[1].real_value;
	return sink_d(a[0].long_value, b);
}

/* v21: non-static helper (cannot be inlined away to a copy) */
real arg_real_extern(union arg const *a, long i)
{
	return a[i].real_value;
}
boolean v21(union arg const *a)
{
	return sink(a[0].long_value, arg_real_extern(a, 1));
}

/* v22: helper in the January calling context */
boolean v22(short fi, long ti, boolean init)
{
	union arg const *a;
	boolean r = 0;

	a = (union arg const *)evaluate(fi, ti, init);
	if (a)
		r = sink(a[0].long_value, arg_real(a, 1));
	return r;
}
