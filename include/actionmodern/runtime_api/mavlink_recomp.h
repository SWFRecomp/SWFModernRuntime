#pragma once

#include <recomp.h>

void recompSITLInit(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompSITLReadPacket(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompSITLSendJSON(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompSITLSendSensor(SWFAppContext* app_context, ASObject* this, u32 num_args);
void recompJSON(SWFAppContext* app_context, ASObject* this, u32 num_args);