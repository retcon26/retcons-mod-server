#pragma once

// PSP file calls on the PC's files (src/platform.c). Paths look like the PSP's, relative to the game
// folder: "data/maps/gm_construct.pmap".

#include "psptypes.h"

#define PSP_O_RDONLY 0x0001
#define PSP_O_WRONLY 0x0002
#define PSP_O_RDWR 0x0003
#define PSP_O_CREAT 0x0200
#define PSP_O_TRUNC 0x0400
#define PSP_SEEK_SET 0
#define PSP_SEEK_CUR 1
#define PSP_SEEK_END 2

#define FIO_S_IFDIR 0x1000
#define FIO_S_ISDIR(m) (((m) & FIO_S_IFDIR) == FIO_S_IFDIR)

typedef struct {
    int st_mode;
    unsigned int st_attr;
    SceOff st_size;
} SceIoStat;

typedef struct {
    SceIoStat d_stat;
    char d_name[256];
    void *d_private;
    int dummy;
} SceIoDirent;

SceUID sceIoOpen(const char *path, int flags, int mode);
int sceIoRead(SceUID fd, void *buf, SceSize size);
int sceIoWrite(SceUID fd, const void *buf, SceSize size);
SceOff sceIoLseek(SceUID fd, SceOff offset, int whence);
int sceIoClose(SceUID fd);
int sceIoGetstat(const char *path, SceIoStat *st);
int sceIoMkdir(const char *path, int mode);
int sceIoRemove(const char *path);
SceUID sceIoDopen(const char *path);
int sceIoDread(SceUID d, SceIoDirent *ent);
int sceIoDclose(SceUID d);
