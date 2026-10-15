#pragma once

// The few PSP kernel calls the engine makes, on a PC (src/platform.c).

#include "pspiofilemgr.h"
#include "psptypes.h"

// Nothing reads the "GE" here, so there's no cache to flush
static inline void sceKernelDcacheWritebackAll(void) {}
static inline void sceKernelDcacheWritebackRange(const void *p, unsigned int size) {
    (void)p;
    (void)size;
}

// Microseconds since the server started
SceInt64 sceKernelGetSystemTimeWide(void);
unsigned int sceKernelGetSystemTimeLow(void);
void sceKernelDelayThread(unsigned int usec);
