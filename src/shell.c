#include "shell.h"
#include "screen.h"
#include "syscall.h"
#include "list.h"

#define BUFF_SIZE      64
#define PROMPT         "D.T.OS >> "
#define KEY_ENTER      0x0D
#define KEY_BACKSPACE  0x08

static char gKBuf[BUFF_SIZE] = {0};
static int  gKIndex = 0;
static List gCmdList = {0};

typedef struct
{
	ListNode header;
	const char* cmd;
	void (*run)();
} CmdRun;

static uint IsKeyDown(uint kc)
{
    return !!(kc & 0xFF000000);
}

static char GetChar(uint kc)
{
    return (char)kc;
}

static byte GetKeyCode(uint kc)
{
    return (byte)(kc >> 8);
}

static void Clear()
{
	int h = 0;
	int w = 0;

	SetPrintPos(CMD_START_W, CMD_START_H);

	for(h=CMD_START_H; h<SCREEN_HEIGHT; h++)
	{
		for(w=CMD_START_W; w<SCREEN_WIDTH; w++) 
		{
			PrintChar(' ');
		}
	}

	SetPrintPos(CMD_START_W, CMD_START_H);
	PrintString(PROMPT);
}

static void AddCmdEntry(const char* cmd, void(*run))
{
	CmdRun* cr = Malloc(sizeof(CmdRun));
	
    //添加命令到命令链表
	if( cr && cmd && run )
	{
		cr->cmd = cmd;
		cr->run = run;

		List_Add(&gCmdList, (ListNode *)cr);
	}
	else
	{
		Free(cr);
	}
}

static int DoCmd(const char* cmd)
{
	int ret = 0;
	ListNode* pos = NULL;
	
    //遍历链表 查找是否有对应命令
	List_ForEach(&gCmdList, pos)
	{
		CmdRun* cr = (CmdRun*)pos;

		if( StrCmp(cmd, cr->cmd, -1) )
		{
			cr->run();
			ret = 1;
			break;
		}
	}

	return ret;
}

static void Unknown(const char* s)
{
	int w = 0;

	SetPrintPos(CMD_START_W, CMD_START_H + 1);

	for(w=CMD_START_W; w<SCREEN_WIDTH; w++)
    {
        PrintChar(' ');
    }

	SetPrintPos(CMD_START_W, CMD_START_H + 1);
	PrintString("Unknow Command: ");
	PrintString(s);	
}

static void ResetCmdLine()
{
	int w = 0;

	SetPrintPos(CMD_START_W, CMD_START_H);

	for(w=CMD_START_W; w<SCREEN_WIDTH; w++)
	{
		PrintChar(' ');
	}

	SetPrintPos(CMD_START_W, CMD_START_H);
	PrintString(PROMPT);
}

static void EnterHandler()
{
	gKBuf[gKIndex++] = 0;

	if( (gKIndex > 1) && !DoCmd(gKBuf) )
	{
		Unknown(gKBuf);
	}

	gKIndex = 0;

    ResetCmdLine();	
}

static void BSHandler()
{
	static int cPtLen = 0;
	int w = GetPrintPosW();

	if( !cPtLen )
	{
		cPtLen = StrLen(PROMPT);
	}
	
    //回退键处理 在长度大于预留字符时 进行删除操作
	if( w > cPtLen )
	{
		w--;

        SetPrintPos(w, CMD_START_H);
        PrintChar(' ');
        SetPrintPos(w, CMD_START_H);		

		gKIndex--;		
	}
}

static void Handle(char ch, byte vk)
{
	if( ch )
	{
		PrintChar(ch);
		gKBuf[gKIndex++] = ch; //保存输入键值
	}
	else
	{
		switch(vk)
		{
			case KEY_ENTER:
				EnterHandler(); //处理回车键
				break;

			case KEY_BACKSPACE:
				BSHandler();
				break;

			default:
			break;
		}
	}
}

static void DoWait()
{
	Delay(5);
	Wait("Deinit");
	ResetCmdLine();
}

static void Demo1()
{
    RunDemo1();
    DoWait();  //等待任务执行完毕
}

static void Demo2()
{
    RunDemo2();
    DoWait();
}


void shell()
{
    List_Init(&gCmdList);

	AddCmdEntry("clear", Clear);
	AddCmdEntry("demo1", Demo1);
	AddCmdEntry("demo2", Demo2);

    SetPrintPos(CMD_START_W, CMD_START_H);
    PrintString(PROMPT);	

    while(1)
    {
		uint key = ReadKey();

		if( IsKeyDown(key) )
		{
            char ch = GetChar(key);
            uint vk = GetKeyCode(key);

			Handle(ch, vk);             
		}
       
    }
}
