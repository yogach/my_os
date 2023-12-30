#include <stdio.h>
#include "fs.h"

int main(void)
{
    HDRawSetName("hd.img");

    HDRawModInit();

    printf("format: %d\n", FSFormat());
    printf("is_formatted: %d\n", FSIsFormatted());
    printf("create: %d\n", FCreate("test.txt"));
    printf("existed: %d\n", FExisted("test.txt"));


    HDRawFlush();

    return 0;
}

