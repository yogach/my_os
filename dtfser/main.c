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
        char str[512] = "TEST";
        char buf[512] = {0};

        if( FExisted(fn) == FS_EXISTED )
        {
            //FDelete(fn);
        }

        //printf("create = %d\n", FCreate(fn));

        uint fd = FOpen(fn);

        printf("fd = %d\n", fd);

        printf("write bytes = %d\n", FWrite(fd, str, sizeof(str)));

        FClose(fd);

        fd = FOpen(fn);

        printf("fd = %d\n", fd);
        printf("pos = %d\n", FTell(fd));
        printf("seek = %d\n", FSeek(fd, 200));
        printf("pos = %d\n", FTell(fd));
        printf("erase = %d\n", FErase(fd, 400));
        printf("len = %d\n", FLength(fd));
        printf("pos = %d\n", FTell(fd));
        printf("seek = %d\n", FSeek(fd, 0));
        printf("read bytes = %d\n", FRead(fd, buf, sizeof(buf)));
        printf("content = %s\n", buf);

        FClose(fd);
        
    }

    HDRawFlush();

    return 0;
}

