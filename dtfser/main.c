#include <stdio.h>
#include "fs.h"

int main(void)
{
    HDRawSetName("hd.img");

    FSModInit();

    //FSFormat();

    if( FSIsFormatted() )
    {
        const char* fn = "test.txt";
        char str[] = "TEST";

        if( FExisted(fn) == FS_EXISTED )
        {
            //FDelete(fn);
        }

        //printf("create = %d\n", FCreate(fn));

        uint fd = FOpen(fn);

        printf("fd = %d\n", fd);

        printf("write bytes = %d\n", FWrite(fd, str, sizeof(str)));

        FClose(fd);

        readFromRoot(fn);
    }

    HDRawFlush();

    return 0;
}

