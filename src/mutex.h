
#ifndef MUTEX_H
#define MUTEX_H

#include "type.h"
#include "queue.h"

enum
{
	Normal,
	Strict	
};

typedef struct 
{
	ListNode head;
	Queue wait;    //用于保存此互斥锁的等待队列
	uint type;     //锁类型
	uint lock;     //锁状态
} Mutex;

void MutexModInit();
void MutexCallHandler(uint cmd, uint param1, uint param2);

#endif
