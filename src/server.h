#pragma once

// The server's settings (server.cfg, then the command line)
typedef struct {
    char game_dir[256]; // a PSP Sandbox folder: the one with data/ in it
    char map[32];
    char name[16];      // shown in the PSPs' Online Servers list
    char build[24];     // a saved build (data/saves/<build>.build) to load where it was built, or ""
    int port;           // UDP
    int lan;            // 1: beacons round the LAN too, so PSPs on it find the server by themselves
    int tick;           // world updates a second
} ServerConfig;

extern ServerConfig g_server;
