#ifndef UTILITY_H
#define UTILITY_H

//这个的作用是统一内核内数组 指针的移位操作
#define AddrOff(a, i) ((void*)((uint)a + i * sizeof(*a)))

void Delay(int n);

#endif
