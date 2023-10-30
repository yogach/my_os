
#ifndef __HDRAW_H__
#define __HDRAW_H__

#include "type.h"

#define SECT_SIZE    512

void HDRawModInit();
uint HDRawSectors();
uint HDRawWrite(uint si, byte* buf);
uint HDRawRead(uint si, byte* buf);

#endif