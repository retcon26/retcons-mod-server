#pragma once

// The subset of the PSP's sceGu graphics API that the engine uses. The dedicated server draws nothing:
// these are empty (src/platform.c), except sceGuGetMemory, which hands out scratch space.
// Constant values match the PSP SDK.

#include "psptypes.h"

#define GU_FALSE 0
#define GU_TRUE 1

// Primitives
#define GU_POINTS 0
#define GU_LINES 1
#define GU_LINE_STRIP 2
#define GU_TRIANGLES 3
#define GU_TRIANGLE_STRIP 4
#define GU_TRIANGLE_FAN 5
#define GU_SPRITES 6

// States
#define GU_ALPHA_TEST 0
#define GU_DEPTH_TEST 1
#define GU_SCISSOR_TEST 2
#define GU_STENCIL_TEST 3
#define GU_BLEND 4
#define GU_CULL_FACE 5
#define GU_DITHER 6
#define GU_FOG 7
#define GU_CLIP_PLANES 8
#define GU_TEXTURE_2D 9
#define GU_LIGHTING 10
#define GU_LIGHT0 11
#define GU_LIGHT1 12
#define GU_LIGHT2 13
#define GU_LIGHT3 14
#define GU_LINE_SMOOTH 15
#define GU_FRAGMENT_2X 21
#define GU_MAX_STATE 22

// Matrix modes
#define GU_PROJECTION 0
#define GU_VIEW 1
#define GU_MODEL 2
#define GU_TEXTURE 3

// Vertex types
#define GU_TEXTURE_SHIFT(n) ((n) << 0)
#define GU_TEXTURE_8BIT GU_TEXTURE_SHIFT(1)
#define GU_TEXTURE_16BIT GU_TEXTURE_SHIFT(2)
#define GU_TEXTURE_32BITF GU_TEXTURE_SHIFT(3)
#define GU_TEXTURE_BITS GU_TEXTURE_SHIFT(3)
#define GU_COLOR_SHIFT(n) ((n) << 2)
#define GU_COLOR_5650 GU_COLOR_SHIFT(4)
#define GU_COLOR_5551 GU_COLOR_SHIFT(5)
#define GU_COLOR_4444 GU_COLOR_SHIFT(6)
#define GU_COLOR_8888 GU_COLOR_SHIFT(7)
#define GU_COLOR_BITS GU_COLOR_SHIFT(7)
#define GU_NORMAL_SHIFT(n) ((n) << 5)
#define GU_NORMAL_8BIT GU_NORMAL_SHIFT(1)
#define GU_NORMAL_16BIT GU_NORMAL_SHIFT(2)
#define GU_NORMAL_32BITF GU_NORMAL_SHIFT(3)
#define GU_NORMAL_BITS GU_NORMAL_SHIFT(3)
#define GU_VERTEX_SHIFT(n) ((n) << 7)
#define GU_VERTEX_8BIT GU_VERTEX_SHIFT(1)
#define GU_VERTEX_16BIT GU_VERTEX_SHIFT(2)
#define GU_VERTEX_32BITF GU_VERTEX_SHIFT(3)
#define GU_VERTEX_BITS GU_VERTEX_SHIFT(3)
#define GU_INDEX_SHIFT(n) ((n) << 11)
#define GU_INDEX_8BIT GU_INDEX_SHIFT(1)
#define GU_INDEX_16BIT GU_INDEX_SHIFT(2)
#define GU_INDEX_BITS GU_INDEX_SHIFT(3)
#define GU_TRANSFORM_3D (0 << 23)
#define GU_TRANSFORM_2D (1 << 23)

// Pixel formats
#define GU_PSM_5650 0
#define GU_PSM_5551 1
#define GU_PSM_4444 2
#define GU_PSM_8888 3
#define GU_PSM_T4 4
#define GU_PSM_T8 5

// Shading, culling
#define GU_FLAT 0
#define GU_SMOOTH 1
#define GU_CW 0
#define GU_CCW 1

// Test functions
#define GU_NEVER 0
#define GU_ALWAYS 1
#define GU_EQUAL 2
#define GU_NOTEQUAL 3
#define GU_LESS 4
#define GU_LEQUAL 5
#define GU_GREATER 6
#define GU_GEQUAL 7

// Clear bits
#define GU_COLOR_BUFFER_BIT 1
#define GU_STENCIL_BUFFER_BIT 2
#define GU_DEPTH_BUFFER_BIT 4
#define GU_FAST_CLEAR_BIT 16

// Texture
#define GU_TFX_MODULATE 0
#define GU_TFX_DECAL 1
#define GU_TFX_BLEND 2
#define GU_TFX_REPLACE 3
#define GU_TFX_ADD 4
#define GU_TCC_RGB 0
#define GU_TCC_RGBA 1
#define GU_NEAREST 0
#define GU_LINEAR 1
#define GU_NEAREST_MIPMAP_NEAREST 4
#define GU_LINEAR_MIPMAP_NEAREST 5
#define GU_NEAREST_MIPMAP_LINEAR 6
#define GU_LINEAR_MIPMAP_LINEAR 7
#define GU_REPEAT 0
#define GU_CLAMP 1
#define GU_TEXTURE_AUTO 0
#define GU_TEXTURE_CONST 1
#define GU_TEXTURE_SLOPE 2

// Blending
#define GU_ADD 0
#define GU_SUBTRACT 1
#define GU_REVERSE_SUBTRACT 2
#define GU_MIN 3
#define GU_MAX 4
#define GU_ABS 5
#define GU_SRC_COLOR 0
#define GU_ONE_MINUS_SRC_COLOR 1
#define GU_SRC_ALPHA 2
#define GU_ONE_MINUS_SRC_ALPHA 3
#define GU_DST_COLOR 0
#define GU_ONE_MINUS_DST_COLOR 1
#define GU_DST_ALPHA 4
#define GU_ONE_MINUS_DST_ALPHA 5
#define GU_FIX 10

// Lighting
#define GU_DIRECTIONAL 0
#define GU_POINTLIGHT 1
#define GU_SPOTLIGHT 2
#define GU_AMBIENT 1
#define GU_DIFFUSE 2
#define GU_SPECULAR 4
#define GU_AMBIENT_AND_DIFFUSE (GU_AMBIENT | GU_DIFFUSE)

// Display lists
#define GU_DIRECT 0
#define GU_CALL 1
#define GU_SEND 2
#define GU_SYNC_FINISH 0
#define GU_SYNC_SIGNAL 1
#define GU_SYNC_DONE 2
#define GU_SYNC_LIST 3
#define GU_SYNC_SEND 4
#define GU_SYNC_WAIT 0
#define GU_SYNC_NOWAIT 1
#define GU_SYNC_WHAT_DONE 0

void sceGuEnable(int state);
void sceGuDisable(int state);
void *sceGuGetMemory(int size);
void sceGuDrawArray(int prim, int vtype, int count, const void *indices, const void *vertices);
void sceGuClear(int flags);
void sceGuClearColor(unsigned int color);
void sceGuClearDepth(unsigned int depth);
void sceGuColor(unsigned int color);
void sceGuAmbient(unsigned int color);
void sceGuLight(int light, int type, int components, const ScePspFVector3 *position);
void sceGuLightColor(int light, int component, unsigned int color);
void sceGuFog(float near, float far, unsigned int color);
void sceGuDepthMask(int mask);
void sceGuDepthFunc(int function);
void sceGuAlphaFunc(int func, int value, int mask);
void sceGuBlendFunc(int op, int src, int dest, unsigned int srcfix, unsigned int destfix);
void sceGuFrontFace(int order);
void sceGuShadeModel(int mode);
void sceGuTexFunc(int tfx, int tcc);
void sceGuTexFilter(int min, int mag);
void sceGuTexWrap(int u, int v);
void sceGuTexScale(float u, float v);
void sceGuTexOffset(float u, float v);
void sceGuTexLevelMode(unsigned int mode, float bias);
void sceGuTexMode(int tpsm, int maxmips, int a2, int swizzle);
void sceGuTexImage(int mipmap, int width, int height, int tbw, const void *tbp);
void sceGuTexFlush(void);
void sceGuClutMode(unsigned int cpsm, unsigned int shift, unsigned int mask, unsigned int a3);
void sceGuClutLoad(int num_blocks, const void *cbp);
