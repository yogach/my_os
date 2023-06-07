
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "type.h"

void MemModInit(byte* mem, uint size);
void* Malloc(uint size);
void Free(void* ptr);


#endif
