#ifndef QUEUE_H
#define QUEUE_H

#include "list.h"

typedef ListNode QueueNode;

typedef struct {
   ListNode head;   //node定义在结构体的头部 使用结构体地址访问时 可直接访问到node
   int length; 
} Queue;

void Queue_Init(Queue* queue);
int Queue_IsEmpty(Queue* queue);
int Queue_IsContained(Queue* queue, QueueNode* node);
void Queue_Add(Queue* queue, QueueNode* node);
QueueNode* Queue_Front(Queue* queue);
QueueNode* Queue_Remove(Queue* queue);
int Queue_Length(Queue* queue);
void Queue_Rotate(Queue* queue);

#endif
