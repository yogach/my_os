
#ifndef MUTEX_H
#define MUTEX_H

#include "type.h"
#include "list.h"

typedef struct 
{
	ListNode head;
	uint lock;     //锁状态
} Mutex;

void MutexModInit();
void MutexCallHandler(uint cmd, uint param);

Mutex* SysCreateMutex();
void SysDestoryMutex(Mutex* mutex);
void SysEnterCritical(Mutex* mutex);
void SysExitCritical(Mutex* mutex);

#endif