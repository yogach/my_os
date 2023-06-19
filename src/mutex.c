
#include "mutex.h"
#include "memory.h"
#include "task.h"

static List gMList = {0};

extern volatile Task* gCTaskAddr;

void MutexModInit()
{
	List_Init(&gMList);
}

void MutexCallHandler(uint cmd, uint param1, uint param2)
{
	if( cmd == 0 )
	{
		uint* pRet = (uint*)param1;

    //将进程锁的地址放入 param对应的地址中 
    //param是用户空间的传入值
		*pRet = (uint)SysCreateMutex();
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

void SysDestroyMutex(Mutex* mutex, uint* result)
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

					PrintString("Destroy Mutex: ");
					PrintIntHex(pos);
				}

				break;
			}
	  }
	}
}

void SysEnterCritical(Mutex* mutex, uint* wait)
{
	if( mutex && IsMutexValid(mutex) )
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
				
				PrintString("Move current to waitting status.\n");
				
				MtxSchedule(WAIT);

			}
		}
		else
		{
			mutex->lock = (uint)gCTaskAddr;  //进入临界区时 使用任务地址作为标识
			
			*wait = 0;

			PrintString("Enter critical section, access critical resource.\n");
		}
	}
}

void SysExitCritical(Mutex* mutex)
{
  PrintString("enter exit\n");

	if( mutex && IsMutexValid(mutex) )
	{
		//释放锁时 如果是任务地址标识不对 则kill掉对应任务
    if( IsEqual(mutex->lock, gCTaskAddr) )
    {
	
			mutex->lock = 0;

			PrintString("Notify all task to run again, critical resource is available.\n");
			MtxSchedule(NOTIFY);
		
    }
		else
		{
			PrintString("enter killtask\n");
			KillTask();
		}
	}
}



