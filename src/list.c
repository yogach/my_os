#include "list.h"

void List_Init(List* list)
{
    list->next = list;
    list->prev = list;
}

//链表插入 node节点插入prev next中间
static void _List_Add(ListNode* node, ListNode* prev, ListNode* next)
{
    next->prev = node;
    node->next = next;
    
    prev->next = node;
    node->prev = prev;
}

//插入链表头
void List_Add(List* list, ListNode* node)
{
    _List_Add(node, list, list->next);
}

//插入链表尾
void List_AddTail(List* list, ListNode* node)
{
    _List_Add(node, list->prev, list);
}

void List_AddBefore(ListNode* before, ListNode* node)
{
    _List_Add(node, before->prev, before);
}

void List_AddAfter(ListNode* after, ListNode* node)
{
    _List_Add(node, after, after->next);
}

static void _List_Del(ListNode* prev, ListNode* next)
{
    prev->next = next;
    next->prev = prev;
}

void List_DelNode(ListNode* node)
{
   _List_Del(node->prev, node->next);
   
   node->prev = NULL;
   node->next = NULL;
}

void List_Replace(ListNode* old, ListNode* node)
{
   node->next = old->next;
   node->next->prev = node;
   node->prev = old->prev;
   node->prev->next = node;
   
   old->next = NULL;
   old->prev = NULL;
    
}

int List_IsLast(List* list, ListNode* node)
{
   return IsEqual(list, node->next);
}

int List_IsEmpty(List* list)
{
   ListNode* next = list->next;
   
   return IsEqual(next, list) && IsEqual(next, list->prev);
}
