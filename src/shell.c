#include "shell.h"
#include "screen.h"
#include "syscall.h"

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