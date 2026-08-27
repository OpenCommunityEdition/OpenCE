extern short val(int);
extern int sink(int);
void probe(short *out)
{
	short x1 = val(1);
	short x2 = val(2);
	short x3 = val(3);
	short x4 = val(4);
	short x5 = val(5);
	short x6 = val(6);
	short x7 = val(7);

	sink(0);
	out[0]=x1; out[1]=x2; out[2]=x3; out[3]=x4; out[4]=x5; out[5]=x6; out[6]=x7;
}
