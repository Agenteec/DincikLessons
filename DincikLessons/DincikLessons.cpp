#include "stdio.h"
struct A
{
	double x, y;
};
struct B
{
	double x, y, z;
};
double f(B* b)
{
	b->z = b->x + b->y;
	return b->x+b->y+b->z;
}
int main() {


		//int** arr = new int* [9];
		//for (size_t i = 0; i < 9; i++)
		//{
		//	arr[i] = new int[9];
		//	for (size_t j = 0; j < 9; j++)
		//	{
		//		arr[i][j] = i * 9 + j;
		//		printf("%c ", arr[i][j]);
		//	}
		//	printf("\n");
		//}
		//arr[0][0] = 5;
		//printf("%c ", *arr[4]);
		//printf("%c ", (char)100);
		A* a = new A();
		a->x = 1;
		a->y = 2;
		printf("%f %f\n", a->x, a->y);
		B* b = (B*)a;
		b->x = 3;
		b->y = 3;
		B* c = new B();
		printf("%f %f\n", b->x, b->y);
		printf("%f\n", f(b));


	return 0;
}


