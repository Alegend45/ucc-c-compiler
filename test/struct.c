#define SIMPLE

#ifdef TIMMY
struct timmy
{
	int i;
	void *p;
};
struct timmy tim; // FIXME

struct
{
	struct
	{
		int i;
	}; // TODO: empty decl or whatever gcc says
	int j;
} tim;
#endif

int
main()
{
#ifdef SIMPLE
	struct
	{
		int a, b;
		struct
		{
			int d, e;
		} sub;
		int c;
	} list;

	list.a = 1;
	list.b = 2;
	list.c = 3;
	list.sub.d = 4;
	list.sub.e = 5;

	return list.sub.d + list.c;

#else
	struct int_struct_ptr
	{
		int i;
		struct ptr_struct
		{
			void *p;
			int i;
		} sub;
		void *p;
	} x;

	x.sub.i = 5;

	return x.sub.i;
#endif
}
