#pragma once

// Video memory, which a server has none of: a block of ordinary memory stands in (src/platform.c)

void *sceGeEdramGetAddr(void);
unsigned int sceGeEdramGetSize(void);
