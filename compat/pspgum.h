#pragma once

// sceGum matrix stack: empty on the server (src/platform.c).

#include "pspgu.h"

void sceGumMatrixMode(int mode);
void sceGumLoadIdentity(void);
void sceGumLoadMatrix(const ScePspFMatrix4 *m);
void sceGumTranslate(const ScePspFVector3 *v);
void sceGumPerspective(float fovy, float aspect, float near, float far);
void sceGumLookAt(ScePspFVector3 *eye, ScePspFVector3 *center, ScePspFVector3 *up);
void sceGumUpdateMatrix(void);
void sceGumDrawArray(int prim, int vtype, int count, const void *indices, const void *vertices);
