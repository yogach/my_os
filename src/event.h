
#ifndef _EVENT_H_
#define _EVENT_H_

#include "type.h"

//声明事件类型
enum
{
    NoneEvent,
    MutexEvent,
    KeyEvent,
    TaskEvent
};

typedef struct
{
    uint type;  //事件类型
    uint id;    //事件标识
    uint param1;
    uint param2;
} Event;

Event* CreateEvent(uint type, uint id, uint param1, uint param2);
void DestroyEvent(Event* event);

#endif