#ifdef A
extern int i = 5; // extern removed
#endif

#ifdef B
static int j = 2;
int j; // error
#endif

#ifdef C
main()
{
	int f(), g();
	//int h();

	f(g(h()));
}
#endif

#ifdef D
int k = 5;
static int k = 2; // error
#endif

#ifdef E
int l;
extern int l; // no effect
#endif

#ifdef F
extern int m;
int m; // m is implicit def
#endif