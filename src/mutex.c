
#include "mutex.h"
#include "memory.h"
#include "task.h"

enum
{
	Normal,
	Strict	
};

typedef struct 
{
	ListNode head;
	uint type;     //锁类型
	uint lock;     //锁状态
} Mutex;


static List gMList = {0};

extern volatile Task* gCTaskAddr;

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


static Mutex* SysCreateMutex(uint type)
{
	Mutex* ret = Malloc(sizeof(Mutex));

	if( ret )
	{
		ret->lock = 0;
		ret->type = type;

		List_Add(&gMList, (ListNode *)ret);
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

static SysNormalEnter(Mutex* mutex, uint* wait)
{
	if( mutex->lock )
	{
		*wait = 1;

		MtxSchedule(WAIT);
	}
	else
	{
    mutex->lock = 1;
	
		*wait = 0;
	}
}

static SysStrictEnter(Mutex* mutex, uint* wait)
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
	
			*wait = 1;
							
			MtxSchedule(WAIT);

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

static SysNormalExit(Mutex* mutex)
{
	mutex->lock = 0;

	MtxSchedule(NOTIFY);
}

static SysStrictExit(Mutex* mutex)
{
	//释放锁时 如果是任务地址标识不对 则kill掉对应任务
	if( IsEqual(mutex->lock, gCTaskAddr) )
	{

		mutex->lock = 0;
		
		MtxSchedule(NOTIFY);
	
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


