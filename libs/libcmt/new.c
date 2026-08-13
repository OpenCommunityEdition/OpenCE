extern "C" void *_nh_malloc(
	unsigned int size,
	int no_throw);

void *operator new(
	unsigned int size)
{
	return _nh_malloc(size, 1);
}
