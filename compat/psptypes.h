#pragma once

// PSP SDK types the engine uses, for the PC build.

#include <stdint.h>

typedef int SceUID;
typedef unsigned int SceSize;
typedef int64_t SceInt64;
typedef int64_t SceOff;
typedef uint32_t SceUInt32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct ScePspFVector2 { float x, y; } ScePspFVector2;
typedef struct ScePspFVector3 { float x, y, z; } ScePspFVector3;
typedef struct ScePspFVector4 { float x, y, z, w; } __attribute__((aligned(16))) ScePspFVector4;
typedef struct ScePspIVector4 { int x, y, z, w; } __attribute__((aligned(16))) ScePspIVector4;

// Row-vector convention, as on the PSP: p' = p.x * x + p.y * y + p.z * z + w
typedef struct ScePspFMatrix4 { ScePspFVector4 x, y, z, w; } ScePspFMatrix4;
typedef struct ScePspIMatrix4 { ScePspIVector4 x, y, z, w; } ScePspIMatrix4;
