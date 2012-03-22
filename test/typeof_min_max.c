#define MIN(A, B)        \
	({                     \
		__typeof(A) a = (A); \
		__typeof(B) b = (B); \
		a < b ? a : b;       \
	})


#define MAX(A, B)        \
	({                     \
		__typeof(A) a = (A); \
		__typeof(B) b = (B); \
		a > b ? a : b;       \
	})


int main()
{
	return MIN(6, 5);
}
