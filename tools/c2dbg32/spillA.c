extern int val(int);
extern int sink(int);
void probe(int *out)
{
	int x1 = val(1);
	int x2 = val(2);

	sink(0);
	out[0] = x1;
	out[1] = x2;
}
