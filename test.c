#include <stdio.h>

union size{
	unsigned short data;
	unsigned char data_char[2];
};

char get_bit(int data, char index) { return index > 7 ? 0 : ((data & (1 << index)) >> index); }

int main(int argc, char **argv)
{
	union size test;
	test.data = 6500;
	printf(" bit value: %i", test.data_char[1]);
	union size test2;
	char i;
	for (i=0; i<2; i++)
		test2.data_char[i] = test.data_char[i];
	printf(" bit value: %i", test.data);
	return 0;

}
