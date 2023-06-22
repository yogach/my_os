#ifndef UTILITY_H
#define UTILITY_H

#include "type.h"


//这个的作用是统一内核内数组 指针的移位操作
#define AddrOff(a, i) ((void*)((uint)(a) + (i) * sizeof(*(a))))
#define AddrIndex(b, a) (((uint)(b) - (uint)(a))/sizeof(*(b)))

#define Max(a, b) ( (a) > (b) ? (a) : (b) )
#define Min(a, b) ( (a) < (b) ? (a) : (b) )


#define IsEqual(a, b)   \
({                      \
    unsigned ta = (unsigned)(a); \
    unsigned tb = (unsigned)(b); \
    !(ta - tb);                \
})

//使用0地址作为假想的type结构体首地址 得到member的偏移地址
#define OffsetOf(type, member) ((unsigned)&(((type*)0)->member))

//首先使用typeof定义一个member成员的指针
//之后使用__mptr指针 - OffsetOf(type, member)) 得到主结构体的地址
#define ContainerOf(ptr, type, member)   \
({                                       \
     const typeof(((type*)0)->member)* __mptr = (ptr); \
     (type*)((char*)__mptr - OffsetOf(type, member));  \
})

void Delay(int n);
char* StrCpy(char* dst, const char* src, int n);
int StrLen(const char* s);
int StrCmp(const char* left, const char* right, uint n);

#endif
