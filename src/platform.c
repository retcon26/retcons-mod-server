// The PSP's platform under the engine, on a PC with no screen, speakers, buttons or microphone.
// Files and time are real (POSIX); graphics, sound, input, voice and the on-screen keyboard do nothing.

#include <dirent.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "audio.h"
#include "gfx.h"
#include "hud.h"
#include "input.h"
#include "mem.h"
#include "osk.h"
#include "pspge.h"
#include "pspgu.h"
#include "pspgum.h"
#include "pspkernel.h"
#include "voice.h"

// --- Time ------------------------------------------------------------------------------------------

static SceInt64 now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (SceInt64)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static SceInt64 g_t0;

SceInt64 sceKernelGetSystemTimeWide(void) {
    if (!g_t0) g_t0 = now_us() - 1;
    return now_us() - g_t0;
}

unsigned int sceKernelGetSystemTimeLow(void) {
    return (unsigned int)sceKernelGetSystemTimeWide();
}

void sceKernelDelayThread(unsigned int usec) {
    usleep(usec);
}

// --- Files -----------------------------------------------------------------------------------------

SceUID sceIoOpen(const char *path, int flags, int mode) {
    int f = 0;
    if ((flags & PSP_O_RDWR) == PSP_O_RDWR) f = O_RDWR;
    else if (flags & PSP_O_WRONLY) f = O_WRONLY;
    else f = O_RDONLY;
    if (flags & PSP_O_CREAT) f |= O_CREAT;
    if (flags & PSP_O_TRUNC) f |= O_TRUNC;
    int fd = open(path, f, mode ? mode : 0644);
    return fd < 0 ? (SceUID)0x80010002 : fd; // the PSP's "no such file"
}

int sceIoRead(SceUID fd, void *buf, SceSize size) {
    return (int)read(fd, buf, size);
}

int sceIoWrite(SceUID fd, const void *buf, SceSize size) {
    return (int)write(fd, buf, size);
}

SceOff sceIoLseek(SceUID fd, SceOff offset, int whence) {
    return lseek(fd, offset, whence == PSP_SEEK_END ? SEEK_END : whence == PSP_SEEK_CUR ? SEEK_CUR : SEEK_SET);
}

int sceIoClose(SceUID fd) {
    return close(fd);
}

int sceIoGetstat(const char *path, SceIoStat *st) {
    struct stat s;
    if (stat(path, &s) != 0) return (int)0x80010002;
    memset(st, 0, sizeof(*st));
    st->st_mode = S_ISDIR(s.st_mode) ? FIO_S_IFDIR : 0;
    st->st_size = s.st_size;
    return 0;
}

int sceIoMkdir(const char *path, int mode) {
    return mkdir(path, (mode_t)mode);
}

int sceIoRemove(const char *path) {
    return unlink(path);
}

// Directory handles: small numbers into this table
#define MAX_DIRS 8
static DIR *g_dirs[MAX_DIRS];

SceUID sceIoDopen(const char *path) {
    for (int i = 0; i < MAX_DIRS; i++) {
        if (g_dirs[i]) continue;
        g_dirs[i] = opendir(path);
        return g_dirs[i] ? i : (SceUID)0x80010002;
    }
    return (SceUID)0x80010018;
}

int sceIoDread(SceUID d, SceIoDirent *ent) {
    if (d < 0 || d >= MAX_DIRS || !g_dirs[d]) return -1;
    struct dirent *e = readdir(g_dirs[d]);
    if (!e) return 0;
    snprintf(ent->d_name, sizeof(ent->d_name), "%s", e->d_name);
    ent->d_stat.st_mode = e->d_type == DT_DIR ? FIO_S_IFDIR : 0;
    return 1;
}

int sceIoDclose(SceUID d) {
    if (d < 0 || d >= MAX_DIRS || !g_dirs[d]) return -1;
    closedir(g_dirs[d]);
    g_dirs[d] = NULL;
    return 0;
}

// --- Graphics: nothing is drawn ------------------------------------------------------------------------

// What the engine builds its vertices in each frame (and never sees again here)
static unsigned char g_gu_mem[1 << 20] __attribute__((aligned(16)));
static size_t g_gu_used;

void *sceGuGetMemory(int size) {
    size = (size + 15) & ~15;
    if (g_gu_used + (size_t)size > sizeof(g_gu_mem)) g_gu_used = 0;
    void *p = g_gu_mem + g_gu_used;
    g_gu_used += (size_t)size;
    return p;
}

// Map loading copies textures into video memory when there's room: there never is here
static unsigned char g_vram[16];
void *sceGeEdramGetAddr(void) {
    return g_vram;
}
unsigned int sceGeEdramGetSize(void) {
    return 0;
}
unsigned int gfx_vram_free_offset(void) {
    return GFX_VRAM_SIZE;
}

void sceGuEnable(int state) { (void)state; }
void sceGuDisable(int state) { (void)state; }
void sceGuDrawArray(int prim, int vtype, int count, const void *indices, const void *vertices) {}
void sceGuClear(int flags) {}
void sceGuClearColor(unsigned int color) {}
void sceGuClearDepth(unsigned int depth) {}
void sceGuColor(unsigned int color) {}
void sceGuAmbient(unsigned int color) {}
void sceGuLight(int light, int type, int components, const ScePspFVector3 *position) {}
void sceGuLightColor(int light, int component, unsigned int color) {}
void sceGuFog(float near, float far, unsigned int color) {}
void sceGuDepthMask(int mask) {}
void sceGuDepthFunc(int function) {}
void sceGuAlphaFunc(int func, int value, int mask) {}
void sceGuBlendFunc(int op, int src, int dest, unsigned int srcfix, unsigned int destfix) {}
void sceGuFrontFace(int order) {}
void sceGuShadeModel(int mode) {}
void sceGuTexFunc(int tfx, int tcc) {}
void sceGuTexFilter(int min, int mag) {}
void sceGuTexWrap(int u, int v) {}
void sceGuTexScale(float u, float v) {}
void sceGuTexOffset(float u, float v) {}
void sceGuTexLevelMode(unsigned int mode, float bias) {}
void sceGuTexMode(int tpsm, int maxmips, int a2, int swizzle) {}
void sceGuTexImage(int mipmap, int width, int height, int tbw, const void *tbp) {}
void sceGuTexFlush(void) {}
void sceGuClutMode(unsigned int cpsm, unsigned int shift, unsigned int mask, unsigned int a3) {}
void sceGuClutLoad(int num_blocks, const void *cbp) {}
void sceGumMatrixMode(int mode) {}
void sceGumLoadIdentity(void) {}
void sceGumLoadMatrix(const ScePspFMatrix4 *m) {}
void sceGumTranslate(const ScePspFVector3 *v) {}
void sceGumPerspective(float fovy, float aspect, float near, float far) {}
void sceGumLookAt(ScePspFVector3 *eye, ScePspFVector3 *center, ScePspFVector3 *up) {}
void sceGumUpdateMatrix(void) {}
void sceGumDrawArray(int prim, int vtype, int count, const void *indices, const void *vertices) {}

void gfx_init(void) {}
void gfx_shutdown(void) {}
void gfx_begin_frame(unsigned int clear_abgr) { g_gu_used = 0; }
void gfx_end_frame(void) {}
void gfx_present(int vsync) {}
void gu_flashlight(int on, const float eye[3], const float fwd[3]) {}
void gfx_world_light(int k, int on, int spot, const float pos[3], const float dir[3], unsigned int abgr, float range) {}
int gfx_light_pass_wanted(void) { return 0; }
int gfx_light_reaches(const float mins[3], const float maxs[3]) { return 0; }
void gfx_light_pass_begin(void) {}
void gfx_light_pass_end(void) {}
void gfx_flash_lit_begin(void) {}
void gfx_flash_lit_end(void) {}
void gfx_set_fog(float near_dist, float far_dist, unsigned int abgr) {}

void hud_init(void) {}
void hud_begin(void) {}
void hud_print(int col, int row, unsigned int abgr, const char *fmt, ...) {}
void hud_print_px(int x, int y, unsigned int abgr, const char *fmt, ...) {}
void hud_rect(int x, int y, int w, int h, unsigned int abgr) {}
void hud_draw(void) {}

// --- Input, sound, voice and the keyboard: none --------------------------------------------------------

void input_init(void) {}
void input_update(InputState *in) {
    memset(in, 0, sizeof(*in));
}

int audio_init(void) { return 0; }
void audio_shutdown(void) {}
int snd_find(const char *name) { return -1; }
int snd_play(int id, float vol, float pan) { return -1; }
int snd_play_chan(int id, float vol, int chan) { return -1; }
int snd_play_at(int id, vec3 pos, float vol) { return -1; }
int snd_loop(int id, float vol) { return -1; }
void snd_stop(int handle) {}
void snd_set_listener(vec3 pos, float yaw_deg) {}
void audio_stream_push(int stream, const short *pcm, int n) {}
int audio_stream_active(int stream) { return 0; }

// Voice chat still reaches everyone: the host passes frames on without playing them (game.c)
int voice_init(void) { return -1; }
int voice_available(void) { return 0; }
void voice_shutdown(void) {}
void voice_set_talking(int on) {}
int voice_talking(void) { return 0; }
int voice_take_frame(VoiceFrame *out) { return 0; }
void voice_play(int stream, const VoiceFrame *f) {}

int osk_start(const char *title, const char *initial, int max_len) { return -1; }
int osk_frame(void) { return 0; }
int osk_active(void) { return 0; }
int osk_result(char *out, int out_len) { return 0; }

// --- Memory: a PC has plenty, so the full-detail packs ----------------------------------------------

void mem_init(void) {}
MemTier mem_tier(void) { return MEM_TIER_HIGH; }
const char *mem_tier_name(void) { return "PC"; }
size_t mem_heap_total(void) { return (size_t)1 << 30; }
size_t mem_heap_used(void) { return 0; }
size_t mem_largest_free(void) { return (size_t)1 << 30; }
