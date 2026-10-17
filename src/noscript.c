// Lua (addons and gamemodes) doesn't run on the server unless it's built with Lua 5.4 (the engine's
// script.c; the Makefile uses it when pkg-config finds lua5.4). Without it, these do nothing, and a
// gamemode in server.cfg falls back to the sandbox.

#include "script.h"

int script_start(const char *map_name, const char *mode, int server, int client) {
    (void)map_name;
    (void)server;
    (void)client;
    return !mode || !mode[0]; // no gamemode to be had
}
void script_stop(void) {}
void script_think(float dt) {
    (void)dt;
}
int script_use(int slot, int body) {
    (void)slot;
    (void)body;
    return 0;
}
void script_hud(void) {}
void script_player_join(int s) {
    (void)s;
}
void script_player_spawn(int s) {
    (void)s;
}
void script_player_death(int s, int attacker) {
    (void)s;
    (void)attacker;
}
void script_player_leave(int s) {
    (void)s;
}
int script_should_damage(int victim, int attacker) {
    (void)victim;
    (void)attacker;
    return 1;
}
void script_entity_damage(int body, int attacker, float amount) {
    (void)body;
    (void)attacker;
    (void)amount;
}
void script_npc_killed(int npc, int type, int attacker) {
    (void)npc;
    (void)type;
    (void)attacker;
}
int script_select_spawn(int s, vec3 *pos, float *yaw) {
    (void)s;
    (void)pos;
    (void)yaw;
    return 0;
}
int script_quick_chat(int s, const char *text) {
    (void)s;
    (void)text;
    return 0;
}
int script_nw_pack(char *out, int max) {
    (void)out;
    (void)max;
    return 0;
}
void script_nw_unpack(const char *in, int len) {
    (void)in;
    (void)len;
}
void script_gamemodes_scan(void) {}
int script_gamemode_count(void) {
    return 1;
}
const char *script_gamemode_id(int i) {
    (void)i;
    return "sandbox";
}
const char *script_gamemode_title(int i) {
    (void)i;
    return "Sandbox";
}
int script_gamemode_find(const char *id) {
    return !id || !id[0] || id[0] == 's' ? 0 : -1;
}
