
#include "mutex.h"
#include "memory.h"
#include "task.h"
#include "event.h"

static List gMList = {0};

extern volatile Task* gCTaskAddr;

static Mutex* SysCreateMutex(uint type)
{
	Mutex* ret = Malloc(sizeof(Mutex));

	if( ret )
	{
		Queue_Init(&ret->wait);
		
		ret->lock = 0;
		ret->type = type;

		List_Add(&gMList, (ListNode *)ret);
	}

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



static void SysDestroyMutex(Mutex* mutex, uint* result)
{
	if( mutex )
	{
		ListNode* pos = NULL;

		*result = 0;

		//检测链表中是否含有这个mutex
	  List_ForEach(&gMList, pos)
		{
			if( IsEqual(pos, mutex) )
			{
				//只有锁已经是释放状态 才能销毁这个锁
				if( IsEqual(mutex->lock, 0) )
				{
					List_DelNode((ListNode*)pos);

					Free(pos);

					*result = 1;

				}

				break;
			}
	  }
	}
}

static void DoWait(Mutex* mutex, uint* wait)
{
	Event* evt = CreateEvent(MutexEvent, (uint)mutex, 0, 0);

	if( evt )
	{
		*wait = 1;
		EventSchedule(WAIT, evt);
	}
}

static void SysNormalEnter(Mutex* mutex, uint* wait)
{
	if( mutex->lock )
	{
		DoWait(mutex, wait);
	}
	else
	{
        mutex->lock = 1;
	
		*wait = 0;
	}
}

static void SysStrictEnter(Mutex* mutex, uint* wait)
{
	if( mutex->lock )
	{
		//同一个任务可以反复的获取锁 而不会进休眠
		if( IsEqual(mutex->lock, gCTaskAddr) )
		{
			*wait = 0;
		}
		else
		{
            DoWait(mutex, wait); //任务进入休眠状态 并将休眠队列放入锁的等待队列中
		}
	}
	else
	{
		mutex->lock = (uint)gCTaskAddr;  //进入临界区时 使用任务地址作为标识
		
		*wait = 0;
	}

}


static void SysEnterCritical(Mutex* mutex, uint* wait)
{
	if( mutex && IsMutexValid(mutex) )
	{
		switch (mutex->type)
		{
			case Normal:
				SysNormalEnter(mutex, wait);
				break;
			case Strict:
				SysStrictEnter(mutex, wait);
				break;
			default:
			  break;
		}
	}
}

static void SysNormalExit(Mutex* mutex)
{
    Event evt = {MutexEvent, (uint)mutex, 0, 0}; //创建一个事件类型

	mutex->lock = 0;

	EventSchedule(NOTIFY, &evt);
}

static void SysStrictExit(Mutex* mutex)
{
	//释放锁时 如果是任务地址标识不对 则kill掉对应任务
	if( IsEqual(mutex->lock, gCTaskAddr) )
	{
    	SysNormalExit(mutex);
	}
	else
	{
		KillTask();
	}

}


static void SysExitCritical(Mutex* mutex)
{

	if( mutex && IsMutexValid(mutex) )
	{
		switch (mutex->type)
		{
			case Normal:
				SysNormalExit(mutex);
				break;
			case Strict:
				SysStrictExit(mutex);
				break;
			default:
			  break;
		}
	}
}

void MutexModInit()
{
	List_Init(&gMList);
}

void MutexCallHandler(uint cmd, uint param1, uint param2)
{
	if( cmd == 0 )
	{
		uint* pRet = (uint*)param1;

        //将进程锁的地址放入 param1对应的地址中     
        //param2 是锁类型
		*pRet = (uint)SysCreateMutex(param2);
	}
	else if( cmd == 1 )
	{
       SysEnterCritical((Mutex*)param1, (uint*)param2);
	}
	else if( cmd == 2 )
	{
       SysExitCritical((Mutex*)param1);
	}
	else
	{
       SysDestroyMutex((Mutex*)param1, (uint*)param2);
	}
}


