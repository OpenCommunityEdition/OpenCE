extern __int64 val(int);
extern int sink(int);
void probe(__int64 *out)
{
	__int64 x1 = val(1);
	__int64 x2 = val(2);
	__int64 x3 = val(3);
	__int64 x4 = val(4);

	sink(0);
	out[0]=x1; out[1]=x2; out[2]=x3; out[3]=x4;
}
