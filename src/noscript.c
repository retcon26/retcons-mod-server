// Lua addons don't run on the server unless it's built with LUA=1 (the engine's script.c and the
// system's Lua 5.4). Without them, these do nothing.

#include "script.h"

void script_start(const char *map_name) {
    (void)map_name;
}
void script_stop(void) {}
void script_think(float dt) {
    (void)dt;
}
int script_use(void) {
    return 0;
}
void script_hud(void) {}
