
#include "hdraw.h"
#include "utility.h"
#include <malloc.h>
#include <stdio.h>

static byte* gHDBuf = NULL;
static uint gSectors = 0;
static char gHDName[64] = {0};

void HDRawSetName(const char* name)
{
    if( name )
    {
        StrCpy(gHDName, name, -1);
    }
}

void HDRawFlush()
{
    FILE* fp = fopen(gHDName, "w");

    if( fp )
    {
        fwrite(gHDBuf, 1, HDRawSectors() * SECT_SIZE, fp);

        fclose(fp);
    }
}

void HDRawModInit()
{
    FILE* fp = fopen(gHDName, "r");

    if( fp )
    {
        long length = 0;

        fseek(fp, 0L, SEEK_END);

        length = ftell(fp);   //计算文件大小

        gHDBuf = (byte*)malloc(length); //按文件大小分配空间

        fseek(fp, 0L, SEEK_SET);

        fread(gHDBuf, 1, length, fp); //将硬盘内容读取到分配到的空间中

        gSectors = length / SECT_SIZE; //得到扇区数

        fclose(fp);
    }
}

uint HDRawSectors()
{
    return gSectors;
}

uint HDRawWrite(uint si, byte* buf)
{
    uint ret = 0;

    if( ret = ((si < HDRawSectors()) && buf) )
    {
        byte* p = AddrOff(gHDBuf, si * SECT_SIZE);

        MemCpy(p, buf, SECT_SIZE);
    }

    return ret;
}

uint HDRawRead(uint si, byte* buf)
{
    uint ret = 0;

    if( ret = ((si < HDRawSectors()) && buf) )
    {
        byte* p = AddrOff(gHDBuf, si * SECT_SIZE);

        MemCpy(buf, p, SECT_SIZE);
    }

    return ret;
}
