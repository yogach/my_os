
#include "screen.h"
#include "kernel.h"

static int  gPosW = 0;
static int  gPosH = 0;
static char gColor = SCREEN_WHITE;

byte GetPrintPosH()
{
    return gPosH;
}

byte GetPrintPosW()
{
    return gPosW;
}


void ClearScreen()
{
    int w = 0;
    int h = 0;
    
    SetPrintPos(w, h);
    
    for(h=0; h<SCREEN_HEIGHT; h++)
    {
      for(w=0; w<SCREEN_WIDTH; w++)
      {
         PrintChar(' ');
      }
    }
    
    SetPrintPos(0, 0);
}

int SetPrintPos(short w, short h)
{
    int ret = 0;
    
    if( ret = ((0 <= w) && (w <= SCREEN_WIDTH) && (0 <= h) && (h <= SCREEN_HEIGHT)) )
    {
        unsigned short bx = SCREEN_WIDTH * h + w;
        
        gPosW = w;
        gPosH = h;
        
        //设置光标位置
        asm volatile (
           "movw %0,      %%bx\n"
           "movw $0x03D4, %%dx\n"
           "movb $0x0e,   %%al\n"
           "outb %%al,    %%dx\n"
           "movw $0x03D5, %%dx\n"
           "movb %%bh,    %%al\n"
           "outb %%al,    %%dx\n"
           "movw $0x03D4, %%dx\n"
           "movb $0x0f,   %%al\n"
           "outb %%al,    %%dx\n"
           "movw $0x03D5, %%dx\n"
           "movb %%bl,    %%al\n"
           "outb %%al,    %%dx\n"           
           :
           : "r"(bx)
           : "ax", "bx", "dx"
        );
    }
    
    return ret;
}


void SetPrintColor(PrintColor c)
{
    gColor = c;
}

int PrintChar(char c)
{
    int ret = 0;
    
    //处理回车换行符 
    if( (c == '\n') || (c == '\r') )
    {
       ret = SetPrintPos(0, gPosH + 1);
    }
    else
    {
        int pw = gPosW;
        int ph = gPosH;
        
        if( (0 <= pw) && (pw <= SCREEN_WIDTH) && (0 <= ph) && (ph <= SCREEN_HEIGHT) )
        {
            int edi = (SCREEN_WIDTH * ph + pw) * 2;
            char ah = gColor;
            char al = c;     
            
            asm volatile(
               "movl %0,   %%edi\n"
               "movb %1,   %%ah\n"
               "movb %2,   %%al\n"
               "movw %%ax, %%gs:(%%edi)\n"
               "\n"
               : 
               : "r"(edi), "r"(ah), "r"(al)
               : "ax",  "edi"
            );
            
            /*asm volatile(
               "mov edi, %[a]\n"
               "mov ah, %[b]   \n"
               "mov al, %[c]  \n"
               "mov gs:[edi], ax \n"
               "\n"
               : 
               : [a]"m"(edi), [b]"m"(ah), [c]"m"(al)
               : "ax",  "edi"
            );*/
            
            pw++;
            
            if(pw == SCREEN_WIDTH)
            {
               pw = 0;
               ph ++;
            }
            
            ret = 1;
        }
        
        SetPrintPos(pw, ph);
    } 
    
    return ret;   
}

int PrintString(const char* s)
{
    int ret = 0;
    
    if( s != NULL )
    {
      while( *s )
      {
        ret += PrintChar(*s++);
      }
    }
    else
    {
      ret = -1;
    }
    
    return ret;
}

//递归调用
int PrintIntDec(int n)
{
    int ret = 0;
    
    if( n < 0 )
    {
       ret += PrintChar('-');
       
       n = -n;
       
       ret += PrintIntDec(n);
    }
    else
    {
       if( n < 10 )
       {
         ret += PrintChar( '0' + n );
       }
       else
       {
         ret += PrintIntDec(n/10);
         ret += PrintIntDec(n%10);
       }
    }
}

int PrintIntHex(unsigned int n)
{
    char hex[11] = {'0', 'x', 0};
    int i = 0;
    
    for(i=9; i>=2; i--)
    {
       int p = n & 0x0f;
       
       if( p < 10 )
       {
          hex[i] = ('0' + p);
       }
       else
       {
          hex[i] = ('A' + p - 10);
       }
       
       n = n >> 4;
    }
    
    return PrintString(hex);
}
