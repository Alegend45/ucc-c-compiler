main()
{
	struct
	{
		char i;
	} a, *b = &a;
	char c;
	char *p = &c;

	*p = 'a';

	c = *p;

	b->i = 5;

	b->i;
}
