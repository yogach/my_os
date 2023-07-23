#include "utility.h"
#include "task.h"
#include "queue.h"
#include "app.h"


#define MAX_TASK_NUM      4
#define MAX_RUNNING_TASK  2
#define MAX_READY_TASK    (MAX_TASK_NUM - MAX_RUNNING_TASK)
#define MAX_TASK_BUFF_NUM (MAX_TASK_NUM + 1)
#define PID_BASE          0x10
#define MAX_TIME_SLICE    260

static AppInfo* (*GetAppToRun) (uint index) = NULL;
static uint (*GetAppNum)() = NULL;

void (* const RunTask)(volatile Task* pt) = NULL;
void (* const LoadTask)(volatile Task* pt) = NULL;

volatile Task* gCTaskAddr = NULL; //使用volatile 防止编译器自动优化此变量 造成指针的值没有变化
static TaskNode gTaskBuff[MAX_TASK_BUFF_NUM] = {0};
static Queue gFreeTaskNode = {0};
static Queue gReadyTask = {0};
static Queue gRunningTask = {0};
static Queue gWaittingTask = {0};
static TSS gTSS = {0};
static TaskNode* gIdleTask = NULL;
static uint gAppToRunIndex = 0;
static uint gPid = PID_BASE;

//所有任务运行的入口
static void TaskEntry()
{
	if( gCTaskAddr )
	{
		gCTaskAddr->tmain();  //调用对应任务函数
	}

	//调用0x80中断
	// to destory current task here
	asm volatile(
	    "movl $0, %eax \n"

	    "int $0x80 \n"
	);
}

//空闲任务 用于在没有任务执行时使用
static void IdleTask()
{
  //这里不屏蔽会造成运行出错 
	//出错的原因是本任务运行在3特权级 而SetPrintPos位于内核代码段中 不可写入
	int i = 0;

	//SetPrintPos(0, 10);

	//PrintString(__FUNCTION__);

	while( 1 );
/*	{
		SetPrintPos(10, 10);
		PrintChar('A' + i);
		i = (i + 1) % 26;
		Delay(1);
	}
*/	
}


static void InitTask(Task* pt, uint id, const char* name, void(*entry)(), ushort pri)
{
	//设置任务相关寄存器
	pt->rv.cs = LDT_CODE32_SELECTOR;
	pt->rv.gs = LDT_VIDEO_SELECTOR;
	pt->rv.ds = LDT_DATA32_SELECTOR;
	pt->rv.es = LDT_DATA32_SELECTOR;
	pt->rv.fs = LDT_DATA32_SELECTOR;
	pt->rv.ss = LDT_DATA32_SELECTOR;   //ss-栈段寄存器

	pt->rv.esp = (uint)pt->stack + AppStackSize;      // esp 栈顶指针寄存器 指向预定义的栈顶 栈地址是从高到低的
	pt->rv.eip = (uint)TaskEntry;                     // eip 下一条指令地址指向任务入口
	pt->rv.eflags = 0x3202;                           // IOPL = 3 if = 1
	pt->tmain = entry;
	pt->id    = id;
	pt->current = 0;
	pt->total = MAX_TIME_SLICE - pri;

	StrCpy(pt->name, name, sizeof(pt->name) - 1);
	
	Queue_Init(&pt->wait);

	//设置局部段描述符
	SetDescValue(AddrOff(pt->ldt, LDT_VIDEO_INDEX),  0xB8000, 0x07FFF, DA_DRWA + DA_32 + DA_DPL3);
	SetDescValue(AddrOff(pt->ldt, LDT_CODE32_INDEX), 0x00,    KernelHeapBase - 1, DA_C + DA_32 + DA_DPL3);
	SetDescValue(AddrOff(pt->ldt, LDT_DATA32_INDEX), 0x00,    KernelHeapBase - 1, DA_DRW + DA_32 + DA_DPL3);

	//设置选择子
	pt->ldtSelector = GDT_TASK_LDT_SELECTOR;
	pt->tssSelector = GDT_TASK_TSS_SELECTOR;

}

static Task* FindTaskByName(const char* name)
{
  Task* ret = NULL;

  //等待的任务不能是IdleTask
  if( !StrCmp(name, "IdleTask", -1) )
  {
    int i = 0;

    for(i=0; i<MAX_TASK_BUFF_NUM; i++)
    {
       TaskNode* tn = AddrOff(gTaskBuff, i);

       //存在id 代表任务已被创建
       if( tn->task.id && StrCmp(tn->task.name, name, -1) )
       {
          ret = &tn->task;
          break;
       }
    }
  
  }  

  return ret;
}

//任务准备执行
static void PrepareForRun(volatile Task* pt)
{
	pt->current ++;

	gTSS.ss0 = GDT_DATA32_FLAT_SELECTOR; //设置0特权级的栈
	gTSS.esp0 = (uint)&pt->rv + sizeof(pt->rv); //esp指针指向 Task结构体RegValue的末尾 目的是在调用中断时自动完成ss esp eflags cs的压栈
	gTSS.iomb = sizeof(TSS);

	//重新设置gdt内的ldt 切换到对应任务的ldt
	SetDescValue(AddrOff(gGdtInfo.entry, GDT_TASK_LDT_INDEX), (uint)&pt->ldt, sizeof(pt->ldt) - 1, DA_LDT + DA_DPL0);
}

static void CreateTask()
{
	uint num = GetAppNum();

	//从空闲队列中取出节点用于注册任务
	while( (gAppToRunIndex < num) && (Queue_Length(&gReadyTask) < MAX_READY_TASK) )
	{
		TaskNode* tn = (TaskNode*)Queue_Remove(&gFreeTaskNode);

		if( tn )
		{
			AppInfo* app = GetAppToRun(gAppToRunIndex);

			InitTask(&tn->task, gPid++, app->name, app->tmain, app->priority);

			Queue_Add(&gReadyTask, (QueueNode*)tn);
		}
		else
		{
			break;
		}

		gAppToRunIndex++;
	}
}

static void CheckRunningTask()
{
	//判断当前执行队列中是否为空 如果为空则需要填入空闲任务
	if( Queue_Length(&gRunningTask) == 0 )
	{
		Queue_Add(&gRunningTask, (QueueNode*)gIdleTask);
	}
	else if( Queue_Length(&gRunningTask) > 1 )
	{
		//当执行队列的任务数量大于1 并且有空闲任务再执行时 移除空闲任务
		if(IsEqual(Queue_Front(&gRunningTask), (QueueNode*)gIdleTask) )
		{
			Queue_Remove(&gRunningTask);
		}
	}

}

static void ReadyToRunning()
{
	QueueNode* node = NULL;

	if( Queue_Length(&gReadyTask) < MAX_READY_TASK)
	{
		CreateTask();
	}

	//将ready的队列放入执行队列
	while( (Queue_Length(&gReadyTask) > 0) && (Queue_Length(&gRunningTask) < MAX_RUNNING_TASK) )
	{
		node = Queue_Remove(&gReadyTask);

		((TaskNode*)node)->task.current = 0;
		Queue_Add(&gRunningTask,  node);
	}

}

static void RunningToReady()
{
	if( Queue_Length(&gRunningTask) > 0 )
	{

		TaskNode* tn = (TaskNode*)Queue_Front(&gRunningTask);

		//不判断idletask的状态
		if( !IsEqual(tn, (QueueNode*)gIdleTask) )
		{

			//如果已经到达运行时间 调度到就绪队列
			if( tn->task.current == tn->task.total )
			{
				Queue_Remove(&gRunningTask);
				Queue_Add(&gReadyTask, (QueueNode*)tn);
			}
		}
	}
}

static void RunningToWaitting(Queue* wq)
{
	if( Queue_Length(&gRunningTask) > 0 )
	{

		TaskNode* tn = (TaskNode*)Queue_Front(&gRunningTask); //得到执行队列的首节点 放入到指定队列

		//非idletask任务才需要进行调度
		if( !IsEqual(tn, (QueueNode*)gIdleTask) )
		{

			Queue_Remove(&gRunningTask);
			Queue_Add(wq, (QueueNode*)tn);

		}
	}

}

static void WaittingToReady(Queue* wq)
{
	//将指定队列中的所有任务都重新加载到就绪队列
	while( Queue_Length(wq) > 0 )
	{
		TaskNode* tn = (TaskNode*)Queue_Front(wq);

		Queue_Remove(wq);
		Queue_Add(&gReadyTask, (QueueNode*)tn);
	}
}


void TaskModInit()
{
	int i = 0;
  byte* pStack = (byte*)(AppHeapBase - (AppStackSize * MAX_TASK_BUFF_NUM)); //将app使用的栈定义在app区域的末尾

  //设置app使用的栈
	for(i=0; i<MAX_TASK_BUFF_NUM; i++)
	{
		TaskNode* tn = (void*)AddrOff(gTaskBuff, i);

		tn->task.stack = (void*)AddrOff(pStack, i * AppStackSize);
	}
  
	gIdleTask = (void*)AddrOff(gTaskBuff, MAX_TASK_NUM);

	//GetAppToRunEntry 强制转换为 uint 类型的指针，并获取其指向的值，再将该值强制转换为 void* 类型的指针。
	GetAppToRun = (void*)(*((uint*)GetAppToRunEntry));
	GetAppNum = (void*)(*((uint*)GetAppNumEntry));

	Queue_Init(&gFreeTaskNode);
	Queue_Init(&gRunningTask);
	Queue_Init(&gReadyTask);
	Queue_Init(&gWaittingTask);

	for(i = 0; i < MAX_TASK_NUM; i++)
	{
		Queue_Add(&gFreeTaskNode, (QueueNode*)AddrOff(gTaskBuff, i));
	}

	//设置全局段描述符内TSS的值
	SetDescValue(AddrOff(gGdtInfo.entry, GDT_TASK_TSS_INDEX), (uint)&gTSS, sizeof(gTSS) - 1, DA_386TSS + DA_DPL0);

	InitTask(&gIdleTask->task, 0, "IdleTask", IdleTask, 255);

	ReadyToRunning();

	CheckRunningTask();

}

static void ScheduleNext()
{
	ReadyToRunning();
	
	CheckRunningTask();
	
	Queue_Rotate(&gRunningTask); //循环 将头节点放入尾部
	
	gCTaskAddr = &((TaskNode*)Queue_Front(&gRunningTask))->task;
	
	PrepareForRun(gCTaskAddr);
	
	LoadTask(gCTaskAddr);
}

void LaunchTask()
{
	gCTaskAddr = &((TaskNode*)Queue_Front(&gRunningTask))->task;

	PrepareForRun(gCTaskAddr);

	//启动任务
	RunTask(gCTaskAddr);
}

void MtxSchedule(uint action)
{
	//当锁状态为等待时 将当前执行任务调度到等待队列中
	if( IsEqual(action, WAIT) )
	{
		RunningToWaitting(&gWaittingTask);

		ScheduleNext();
	}
	else if( IsEqual(action, NOTIFY) )
	{
		WaittingToReady(&gWaittingTask);
	}
}


//任务调度函数 位于时钟中断内
void Schedule()
{
	RunningToReady();

	ScheduleNext();
}

void KillTask()
{
	//PrintString(__FUNCTION__);  // destroy current task

	//进入此处代表当前任务已经运行结束 将节点从运行队列中取出 重新放入空闲队列中
	QueueNode* node = Queue_Remove(&gRunningTask);
  Task* task = &((TaskNode*)node)->task;   

  WaittingToReady(&task->wait);  //同时将等待此任务完成运行的任务 重新加载到就绪队列

	task->id = 0;

	Queue_Add(&gFreeTaskNode, node);

	Schedule();
}

void WaitTask(const char* name )
{
   Task* task = FindTaskByName(name); //查找需要等待的任务是否存在

   if( task )
   {
     RunningToWaitting(&task->wait);
     ScheduleNext();
   }
}

void TaskCallHandler(uint cmd, uint param1, uint param2 )
{
	switch (cmd)
	{
  	case 0:
  	  KillTask();
  	break;

  	case 1:
      WaitTask((char*)param1); 
  	break;
    
	default:
	break; 
	}
}


