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
} ServerConfig;

extern ServerConfig g_server;

// The server list (src/listserver.c)
int list_server_start(void);
void list_server_stop(void);
void list_server_poll(void (*log)(const char *msg));
int list_server_status(void (*log)(const char *msg)); // logs each server; returns how many
