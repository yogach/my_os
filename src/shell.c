#include "shell.h"
#include "screen.h"
#include "syscall.h"

#define BUFF_SIZE      64
#define PROMPT         "D.T.OS >> "
#define KEY_ENTER      0x0D
#define KEY_BACKSPACE  0x08

static char gKBuf[BUFF_SIZE] = {0};
static int  gKIndex = 0;
static List gCmdList = {0};

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

static void clear()
{
	
}

static void AddCmdEntry(const char* cmd, void(*run))
{
	
}

static int DoCmd(const char* cmd)
{
	
}

static void Unknown(const char* s)
{
	
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

void shell()
{
    SetPrintPos(0, 9);
    PrintString("D.T.OS >> ");

    while(1)
    {
		uint key = ReadKey();

		if( IsKeyDown(key) )
		{
            char ch = GetChar(key);
            uint vk = GetKeyCode(key);
            
            if( ch )
            {
                PrintChar(ch);
            } 
		}
       
    }
}
