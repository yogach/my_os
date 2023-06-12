#ifndef IHANDLER_H
#define IHANDLER_H

#include "type.h"

#define DeclHandler(name)     void name##Entry(); \
                              void name()

DeclHandler(SegmentFaultHandler);
DeclHandler(PageFaultHandler);
DeclHandler(TimerHandler);
DeclHandler(SysCallHandler);

                              
#endif
