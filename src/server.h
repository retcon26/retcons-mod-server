#pragma once

// The server's settings (server.cfg, then the command line)
typedef struct {
    char mode[8];       // "game": host a map; "list": a server list for game servers and PSPs
    char game_dir[256]; // the game's folder: the one with data/ in it
    char map[32];
    char name[16];      // shown in the PSPs' Online Servers list
    char build[24];     // a saved build (data/saves/<build>.build) to load where it was built, or ""
    char list[64];      // a server list to be listed on (host or host:port), or ""
    char public_address[32]; // the address the list gives players, if not the one it sees ("": that one)
    int port;           // UDP
    int list_port;      // mode list: its UDP port
    int lan;            // 1: beacons round the LAN too, so PSPs on it find the server by themselves
    int tick;           // world updates a second
    int max_players;    // 1 to 7: the game's 8 slots, less the one the server keeps
    char password[16];  // players need it to join ("": anyone can)
    char maps[512];     // the map rotation: map names with spaces between ("": just map)
    int map_minutes;    // minutes on each map before the next in maps (0: no rotation)
    int autosave_minutes; // the players' build saved this often (0: never)
    int autosave_resume;  // 1: a map starts with its last autosave
} ServerConfig;

extern ServerConfig g_server;

// The server list (src/listserver.c)
int list_server_start(void);
void list_server_stop(void);
void list_server_poll(void (*log)(const char *msg));
int list_server_status(void (*log)(const char *msg)); // logs each server; returns how many
