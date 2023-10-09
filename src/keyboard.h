
#ifndef __KEY_BOARD_H__
#define __KEY_BOARD_H__

#include "type.h"

void KeyboardModInit();
void PutScanCode(byte sc);
uint FetchKeyCode();
void KeyCallHandler(uint cmd, uint param1, uint param2);
void NotifyKeyCode();



#endif
