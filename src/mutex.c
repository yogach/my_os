
#include "mutex.h"
#include "memory.h"

static List gMList = {0};

void MutexModInit()
{
	List_Init(&gMList);
}

void MutexCallHandler(uint cmd, uint param)
{
	if( cmd == 0 )
	{
		uint* pRet = (uint*)param;

    //将进程锁的地址放入 param对应的地址中 
    //param是用户空间的传入值
		*pRet = (uint*)SysCreateMutex();
	}
	else if( cmd == 1 )
	{
    SysEnterCritical((Mutex*)param);
	}
	else if( cmd == 2 )
	{
    SysExitCritical((Mutex*)param);
	}
	else
	{
    SysDestoryMutex((Mutex*)param);
	}
}

Mutex* SysCreateMutex()
{
	Mutex* ret = Malloc(sizeof(Mutex));

	if( ret )
	{
		ret->lock = 0;

		List_Add(&gMList, (ListNode *)ret);
	}

	PrintString("Mutex ID: ");
	PrintIntHex(ret);
	PrintChar('\n');


	return ret;
}

static uint IsMutexValid(Mutex* mutex)
{
	uint ret = 0;
	ListNode* pos = NULL;

  //检测链表中是否含有这个mutex
	List_ForEach(&gMList, pos)
	{
		if( IsEqual(pos, mutex) )
		{
			ret = 1;
			break;
		}
	}

	return ret;
}

void SysDestoryMutex(Mutex* mutex)
{
	if( mutex )
	{
		ListNode* pos = NULL;

		//检测链表中是否含有这个mutex
	  List_ForEach(&gMList, pos)
		{
			if( IsEqual(pos, mutex) )
			{
				List_DelNode((ListNode*)pos);

				Free(pos);

				PrintString("Destroy Mutex: ");
				PrintIntHex(pos);

				break;
			}
	  }
	}
}

void SysEnterCritical(Mutex* mutex)
{
	if( mutex && IsMutexValid(mutex) )
	{
		if( mutex->lock )
		{
			PrintString("Move current to waitting status.\n");
		}
		else
		{
			mutex->lock = 1;

			PrintString("Enter critical section, access critical resource.\n");
		}
	}
}

void SysExitCritical(Mutex* mutex)
{
	if( mutex && IsMutexValid(mutex) )
	{
		mutex->lock = 0;

		PrintString("Notify all task to run again, critical resource is available.\n");
	}
}



