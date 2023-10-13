
#include <stdio.h>

// can't be pointers tho ...
const char CONFIG_STR_VALUE[] = "Hello world!";
const char CONFIG_STR_EMPTY[32];

int main()
{
	printf("'%s':'%s'\n", CONFIG_STR_VALUE, CONFIG_STR_EMPTY);

	return 0;
}
