
#include "utility.h"

void Delay(int n)
{
    while( n > 0 )
    {
       int i=0;
       int j=0;
       
       for(i=0; i<1000; i++)
        {
            for(j=0; j<1000; j++)
            {
              asm volatile ("nop \n");
            }
        }
        
        n --;
    }
}

byte* MemCpy(byte* dst, const byte* src, uint n)
{
	byte* ret = dst;
	uint daddr = (uint)dst;
	uint saddr = (uint)src;
	int i = 0;

	if( daddr < saddr )
	{
		for(i=n-1; i>=0; i--)
		{
			dst[i] = src[i];
		}
	}
	else if( daddr > saddr )
	{
		for(i=0; i<n; i++)
		{
			dst[i] = src[i];
		}
	}

	return ret;
}

byte* MemSet(byte* dst, uint n, byte val)
{
	byte* ret = dst;
    int i;

    while(i < n)
    {
		dst[i] = val;
		i++;
    }

	return ret;
}


char* StrCpy(char* dst, const char* src, uint n)
{
  char* ret = dst;
  uint daddr = (uint)dst;
  uint saddr = (uint)src;
  int i = 0;
 
  if( daddr < saddr )
  {
  	//首先计算字符串长度 得到末尾位置
	uint len = StrLen(src);
	
	len = (len < n) ? len : n;
	
	dst[len] = 0;
  
	for(i=len-1; i>=0 ; i--)
	{
		dst[i] = src[i];
	}
	
  }
  else if( daddr > saddr )
  {
	for(i=0; src[i] && (i < n); i++)
	{
		dst[i] = src[i];
	}
	
    dst[i] = 0; //结束符
  }  
 
  return ret;
}

int StrLen(const char* s)
{
   int ret = 0;

   while(s && ( s[ret] != '\0') )
   {
   	 ret++;
   }

   return ret;
}

int StrCmp(const char* left, const char* right, uint n)
{
   int ret = 1;

   //首先判断字符串地址是否一致 一致则不用比较
   if( !IsEqual(left, right) )
   {
    int lLen = StrLen(left);
	  int rLen = StrLen(right);
	  int m = Min(lLen, rLen);
	  int i = 0;

	  n = Min(m, n);   

	  ret = IsEqual(lLen, rLen); //只有长度相同才能比较？ 

	  for(i=0; (i<n) && ret ; i++)
	  {
	    ret = IsEqual(left[i], right[i]);

			if( !ret )
			{
			   break;
			}
	  }
   }

   return ret;
}

