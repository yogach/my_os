
#include "app.h"
#include "utility.h"
#include "memory.h"
#include "syscall.h"
#include "demo1.h"
#include "demo2.h"

#define MAX_APP_NUM   16

static AppInfo gAppToRun[MAX_APP_NUM] = {0};
static uint gAppNum = 0;

int i = 0;
uint g_mutex = 0;



void TaskA();
void TaskB();
void TaskC();
void TaskD();

void CookRice();
void CookDish();
void HaveDinner();

/*
static void RegApp(const char* name, void(*tmain)(), byte pri)
{

}
*/

void AppMain()
{
    int* p = (int*)0x80000;
    
    *p = 0;
    
    SetPrintPos(0, 10);
    PrintString("AppMain() : Hello D.T.OS!\n");
}


void CookRice()
{
	int i = 0;
	
	SetPrintPos(0, 12);
	
	PrintString(__FUNCTION__);
	PrintChar('\n');

	for(i=0; i<50; i++)
	{
			SetPrintPos(10, 12);
			PrintChar('A' + i % 26);
			Delay(1);
	}

}

void CookDish()
{
	int i = 0;
	
	SetPrintPos(0, 14);
	
	PrintString(__FUNCTION__);
	PrintChar('\n');

	for(i=0; i<30; i++)
	{
			SetPrintPos(10, 14);
			PrintChar('0' + i % 10);
			Delay(1);
	}


}

void HaveDinner()
{
  Wait("CookDish");
  Wait("CookRice");

	SetPrintPos(10, 16);
	PrintString("Having dinner...\n");
	
}

void TaskA()
{    
    SetPrintPos(0, 12);
    
    PrintString(__FUNCTION__);
    PrintChar('\n');
    
    PrintIntDec(StrLen("a"));
    PrintChar('\n');
    
    char a[] = "abcd";
    char b[] = "abc";
    
    PrintIntDec(StrCmp(a, b, 3));
    PrintChar('\n');  
}

void TaskB()
{
	SetPrintPos(0, 16);
	
	PrintString(__FUNCTION__);
	PrintChar('\n');  

  //ExitCritical(g_mutex);  //测试没有获取锁时 释放锁

  EnterCritical(g_mutex);
  
  i = 0;
  
	while(0)
	{
			SetPrintPos(8, 16);
			PrintChar('0' + i);
			i = (i + 1) % 10;
			Delay(1);
	}
	
	SetPrintPos(8, 16);

	//测试没有释放锁时 销毁锁	
	i = DestroyMutex(g_mutex);
	
	PrintIntDec(i);
	PrintChar('\n');
	
	ExitCritical(g_mutex);
	
	i = DestroyMutex(g_mutex);
	
	PrintIntDec(i);
	PrintChar('\n');

}

void TaskC()
{
	int i = 0;

	SetPrintPos(0, 14);

	PrintString(__FUNCTION__);

	while(1)
	{
		SetPrintPos(8, 14);
		PrintChar('a' + i);
		i = (i + 1) % 26;
		Delay(1);
	}
}

void TaskD()
{
	int i = 0;

	SetPrintPos(0, 15);

	PrintString(__FUNCTION__);

	while(1)
	{
		SetPrintPos(8, 15);
		PrintChar('!' + i);
		i = (i + 1) % 10;
		Delay(1);
	}
}
