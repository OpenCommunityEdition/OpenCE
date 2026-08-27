extern char val(int);
extern int sink(int);
void probe(char *out)
{
	char x1 = val(1);
	char x2 = val(2);
	char x3 = val(3);
	char x4 = val(4);
	char x5 = val(5);
	char x6 = val(6);
	char x7 = val(7);

	sink(0);
	out[0]=x1; out[1]=x2; out[2]=x3; out[3]=x4; out[4]=x5; out[5]=x6; out[6]=x7;
}
