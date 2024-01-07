#include <stdio.h>
#include "fs.h"

int main(void)
{
    HDRawSetName("hd.img");

    HDRawModInit();

    printf("format: %d\n", FSFormat());
    printf("is_formatted: %d\n", FSIsFormatted());
    printf("create: %d\n", FCreate("1.txt"));
    printf("create: %d\n", FCreate("2.txt"));
	printf("create: %d\n", FCreate("3.txt"));
	printf("create: %d\n", FCreate("4.txt"));
	printf("create: %d\n", FCreate("5.txt"));

    printf("delete: %d\n", FDelete("3.txt"));
    printf("existed: %d\n", FExisted("3.txt"));

	listFile();


    HDRawFlush();

    return 0;
}

