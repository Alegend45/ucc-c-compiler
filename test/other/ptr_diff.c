#define abs(x) \
	({ int i = (x); i < 0 ? -i : i; })

main()
{
	int a, b;
	int *pa, *pb;

	pa = &a;
	pb = &b;

	assert(abs(pb - pa) == 1);
}
