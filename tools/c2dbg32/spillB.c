extern int val(int);
extern int sink(int);
void probe(int *out)
{
	int x1 = val(1);
	int x2 = val(2);
	int x3 = val(3);
	int x4 = val(4);
	int x5 = val(5);
	int x6 = val(6);
	int x7 = val(7);

	sink(0);
	out[0] = x1;
	out[1] = x2;
	out[2] = x3;
	out[3] = x4;
	out[4] = x5;
	out[5] = x6;
	out[6] = x7;
}
