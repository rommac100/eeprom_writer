#include <stdio.h>

int main(int argc, char **argv)
{
	int size_bits = 5;	
	int size_byte = (((double) size_bits)/8.00)+1;
	printf("size scaled: %i\n", size_byte);
	return 0;
}
