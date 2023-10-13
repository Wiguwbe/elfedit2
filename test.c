
#include <stdio.h>
#include <stdint.h>

/*
	make `volatile` to avoid
	unwanted optimizations
	(always read from memory instead
	of using literals(
*/
const volatile int CONFIG_INT = 4;
const volatile short CONFIG_NEG = -2;
const volatile int8_t CONFIG_BYTE = 66;
const volatile long CONFIG_LONG = 616;

int main()
{
	printf("%ld %d %hd %hhd\n", CONFIG_LONG, CONFIG_INT, CONFIG_NEG, CONFIG_BYTE);
	return 0;
}
