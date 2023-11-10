#include <stdio.h>
#include "hdraw.h"

int main(void)
{
    HDRawSetName("hd.img");

    HDRawModInit();

    byte buf[512] = {0};

    HDRawRead(2, buf);

    printf("%x, %x, %x\n", buf[1], buf[128], buf[511]);

    HDRawFlush();

    return 0;
}

