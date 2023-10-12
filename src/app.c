
#include "utility.h"
#include "memory.h"
#include "syscall.h"
#include "demo1.h"
#include "demo2.h"
#include "shell.h"

int i = 0;
uint g_mutex = 0;

void TaskA();
void TaskB();
void TaskC();
void TaskD();

void CookRice();
void CookDish();
void HaveDinner();

void AppMain()
{
	//RegApp("CookRice", CookRice, 255);
	//RegApp("CookDish", CookDish, 255);
	//RegApp("HaveDinner", HaveDinner, 255);

	//RunDemo1();
	//RunDemo2();	

	RegApp("shell", shell, 255);
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

