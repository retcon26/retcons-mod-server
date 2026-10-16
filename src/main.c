// Retcon's Mod dedicated server (formerly PSP Sandbox's): runs a map with the game's own engine (physics,
// NPCs, doors, saved builds) and hosts it for PSPs over Wi-Fi and the internet. Nobody plays on the
// server itself. With "mode list" it's a server list instead (src/listserver.c).
//
//   psp-sandbox-server [-c server.cfg] [--game DIR] [--map NAME] [--port N] [--name NAME] [--mode list]
//
// Type commands while it runs: help, status, say, kick, map, save, load, cleanup, quit.

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "game.h"
#include "mem.h"
#include "net.h"
#include "pspkernel.h"
#include "server.h"
#include "settings.h"
#include "version.h"

#define SERVER_VERSION "1.1.1"
#define HEARTBEAT_US 30000000 // how often a game server tells its server list it's still up

ServerConfig g_server = {
    .mode = "game",
    .game_dir = ".",
    .map = "gm_construct",
    .name = "Retcon's Server",
    .port = NET_PORT,
    .list_port = NET_LIST_PORT,
    .lan = 1,
    .tick = 60,
};

static volatile sig_atomic_t g_quit;
static int g_in_game;
static int g_list_mode;

static void on_signal(int sig) {
    (void)sig;
    g_quit = 1;
}

static void logf_(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void logf_(const char *fmt, ...) {
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    printf("[%02d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);
}

// The game's notes (who joined, who left, chat) go to the log
static void game_log(const char *msg) {
    logf_("%s", msg);
}

// --- Settings -----------------------------------------------------------------------------------------

static void trim(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = 0;
}

// One "key value" setting (from server.cfg or the command line); 0 if the key isn't one
static int apply_setting(const char *key, const char *value) {
    if (strcmp(key, "mode") == 0) {
        snprintf(g_server.mode, sizeof(g_server.mode), "%s", value);
    } else if (strcmp(key, "list") == 0) {
        snprintf(g_server.list, sizeof(g_server.list), "%s", value);
    } else if (strcmp(key, "public_address") == 0) {
        snprintf(g_server.public_address, sizeof(g_server.public_address), "%s", value);
    } else if (strcmp(key, "list_port") == 0) {
        g_server.list_port = atoi(value);
    } else if (strcmp(key, "game_dir") == 0 || strcmp(key, "game") == 0) {
        snprintf(g_server.game_dir, sizeof(g_server.game_dir), "%s", value);
    } else if (strcmp(key, "map") == 0) {
        snprintf(g_server.map, sizeof(g_server.map), "%s", value);
    } else if (strcmp(key, "name") == 0) {
        snprintf(g_server.name, sizeof(g_server.name), "%s", value);
    } else if (strcmp(key, "build") == 0) {
        snprintf(g_server.build, sizeof(g_server.build), "%s", value);
    } else if (strcmp(key, "port") == 0) {
        g_server.port = atoi(value);
    } else if (strcmp(key, "lan") == 0) {
        g_server.lan = atoi(value);
    } else if (strcmp(key, "tick") == 0) {
        g_server.tick = atoi(value);
    } else if (strcmp(key, "god_mode") == 0) {
        g_settings.god_mode = atoi(value);
    } else if (strcmp(key, "npc_think") == 0) {
        g_settings.npc_think = atoi(value);
    } else if (strcmp(key, "npc_ignore_players") == 0) {
        g_settings.npc_ignore_players = atoi(value);
    } else if (strcmp(key, "unlimited_ammo") == 0) {
        g_settings.unlimited_ammo = atoi(value);
    } else if (strcmp(key, "physics") == 0) {
        g_settings.physics = atoi(value);
    } else {
        return 0;
    }
    return 1;
}

static int load_config(const char *path, int required) {
    FILE *f = fopen(path, "r");
    if (!f) {
        if (required) fprintf(stderr, "Can't read %s: %s\n", path, strerror(errno));
        return required ? -1 : 0;
    }
    char line[512];
    int n = 0;
    while (fgets(line, sizeof(line), f)) {
        n++;
        char *s = line;
        while (*s == ' ' || *s == '\t') s++;
        char *hash = strchr(s, '#');
        if (hash) *hash = 0;
        trim(s);
        if (!*s) continue;
        char *v = s + strcspn(s, " \t");
        if (*v) *v++ = 0;
        while (*v == ' ' || *v == '\t') v++;
        if (!apply_setting(s, v)) fprintf(stderr, "%s:%d: unknown setting '%s'\n", path, n, s);
    }
    fclose(f);
    return 0;
}

static void usage(void) {
    printf("Retcon's Mod dedicated server %s (game %s)\n\n", SERVER_VERSION, GAME_VERSION);
    printf("  psp-sandbox-server [-c server.cfg] [--game DIR] [--map NAME] [--port N] [--name NAME]\n");
    printf("  psp-sandbox-server --mode list [--list_port N]     (a server list)\n\n");
    printf("DIR is the game's folder (the one with data/ in it). Settings in server.cfg, see server.cfg.example.\n");
}

// --- The server list: a game server's heartbeats ------------------------------------------------------

static uint8_t g_list_addr[6];
static int g_list_ok; // 1 resolved, -1 couldn't be (tried again at the next heartbeat)
static SceInt64 g_next_heartbeat;

static void heartbeat(void) {
    if (!g_server.list[0] || !net_active()) return;
    SceInt64 now = sceKernelGetSystemTimeWide();
    if (now < g_next_heartbeat) return;
    g_next_heartbeat = now + HEARTBEAT_US;
    if (g_list_ok != 1) {
        char text[80];
        snprintf(text, sizeof(text), strchr(g_server.list, ':') ? "%s" : "%s:18257", g_server.list);
        int was = g_list_ok;
        g_list_ok = net_resolve(text, g_list_addr) == 0 ? 1 : -1;
        if (g_list_ok != 1) {
            if (was == 0) logf_("Can't find the server list %s (trying again every 30 seconds)", g_server.list);
            return;
        }
        logf_("Listing this server on %s", g_server.list);
    }
    PkHeartbeat hb;
    memset(&hb, 0, sizeof(hb));
    hb.h = (NetHeader){NET_MAGIC, PK_HEARTBEAT, 0, NET_VERSION};
    // The address players should use: public_address if it's set, else the one the list sees us at
    if (g_server.public_address[0]) {
        uint8_t a[6];
        char text[48];
        snprintf(text, sizeof(text), "%s:%d", g_server.public_address, g_server.port);
        if (net_resolve(text, a) == 0) memcpy(hb.addr, a, 4);
    }
    hb.addr[4] = (uint8_t)(g_server.port >> 8);
    hb.addr[5] = (uint8_t)(g_server.port & 0xFF);
    net_send(g_list_addr, &hb, sizeof(hb));
}

// --- Maps ---------------------------------------------------------------------------------------------

static int start_map(const char *map) {
    char err[96] = "";
    if (game_start(map, err, sizeof(err)) != 0) {
        logf_("Can't start %s: %s", map, err);
        return -1;
    }
    g_in_game = 1;
    game_host_dedicated(game_log);
    g_next_heartbeat = 0; // tell the server list straight away
    logf_("Hosting %s as \"%s\" on UDP port %d", map, g_server.name, g_server.port);
    if (!net_active()) logf_("Network: %s", net_status());
    return 0;
}

static void end_map(int reason) {
    if (!g_in_game) return;
    for (int s = 1; s < NET_MAX_PLAYERS; s++) game_kick(s, reason);
    game_end();
    g_in_game = 0;
}

// --- Console ------------------------------------------------------------------------------------------

static void cmd_status(void) {
    logf_("%s - %s - port %d", g_server.name, g_in_game ? g_server.map : "(no map)", g_server.port);
    int n = 0;
    for (int s = 1; s < NET_MAX_PLAYERS; s++) {
        const char *name = game_peer_name(s);
        if (!name) continue;
        logf_("  slot %d: %s", s, name);
        n++;
    }
    logf_("%d of %d players", n, NET_MAX_PLAYERS - 1);
}

static void console_line(char *line) {
    trim(line);
    char *arg = line + strcspn(line, " ");
    if (*arg) *arg++ = 0;
    while (*arg == ' ') arg++;
    if (!*line) return;
    if (g_list_mode) {
        if (strcmp(line, "status") == 0) {
            logf_("Server list on UDP port %d", g_server.list_port);
            logf_("%d servers listed", list_server_status(game_log));
        } else if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            g_quit = 1;
        } else {
            logf_("status | quit");
        }
        return;
    }
    if (strcmp(line, "help") == 0) {
        logf_("status | say TEXT | kick SLOT | map NAME | save NAME | load NAME | cleanup | quit");
    } else if (strcmp(line, "status") == 0) {
        cmd_status();
    } else if (strcmp(line, "say") == 0 && *arg) {
        char text[64];
        snprintf(text, sizeof(text), "Server: %s", arg);
        game_chat(text);
    } else if (strcmp(line, "kick") == 0 && *arg) {
        int s = atoi(arg);
        if (!game_peer_name(s)) {
            logf_("Nobody in slot %d (see status)", s);
        } else {
            game_kick(s, GAME_LEAVE_KICKED);
        }
    } else if (strcmp(line, "map") == 0 && *arg) {
        char map[32];
        snprintf(map, sizeof(map), "%s", arg);
        end_map(GAME_LEAVE_MAP);
        if (start_map(map) == 0) {
            snprintf(g_server.map, sizeof(g_server.map), "%s", map);
        } else if (start_map(g_server.map) != 0) {
            g_quit = 1;
        }
    } else if (strcmp(line, "save") == 0 && *arg) {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "savebuild %s", arg);
        game_command(cmd);
    } else if (strcmp(line, "load") == 0 && *arg) {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "loadhere %s", arg); // where it was built (the server has no view to aim)
        game_command(cmd);
    } else if (strcmp(line, "cleanup") == 0) {
        game_command("cleanup");
    } else if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
        g_quit = 1;
    } else {
        logf_("Unknown command '%s' (try help)", line);
    }
}

// Reads whatever's been typed, a line at a time, without waiting
static void console_poll(void) {
    static char buf[256];
    static size_t used;
    static int closed; // stdin closed (running in the background): no console
    struct pollfd p = {0, POLLIN, 0};
    while (!closed && poll(&p, 1, 0) > 0 && (p.revents & (POLLIN | POLLHUP))) {
        ssize_t n = read(0, buf + used, sizeof(buf) - 1 - used);
        if (n <= 0) {
            closed = 1;
            return;
        }
        used += (size_t)n;
        buf[used] = 0;
        char *nl;
        while ((nl = strchr(buf, '\n')) != NULL) {
            *nl = 0;
            console_line(buf);
            size_t rest = used - (size_t)(nl + 1 - buf);
            memmove(buf, nl + 1, rest + 1);
            used = rest;
        }
        if (used >= sizeof(buf) - 1) used = 0; // a line too long to be a command
    }
}

// --- Main ---------------------------------------------------------------------------------------------

int main(int argc, char **argv) {
    const char *cfg = NULL;
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) && i + 1 < argc) {
            cfg = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        }
    }
    if (load_config(cfg ? cfg : "server.cfg", cfg != NULL) != 0) return 1;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--", 2) == 0 && i + 1 < argc && strcmp(argv[i], "--config") != 0) {
            if (!apply_setting(argv[i] + 2, argv[i + 1])) {
                fprintf(stderr, "Unknown option %s\n", argv[i]);
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            i++;
        }
    }
    if (g_server.tick < 10 || g_server.tick > 240) g_server.tick = 60;
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGPIPE, SIG_IGN);
    setvbuf(stdout, NULL, _IOLBF, 0);

    if (strcmp(g_server.mode, "list") == 0) {
        // A server list: no game, no game data
        g_list_mode = 1;
        logf_("Retcon's Mod server list %s (game protocol %d)", SERVER_VERSION, NET_VERSION);
        if (list_server_start() != 0) {
            logf_("Can't open UDP port %d", g_server.list_port);
            return 1;
        }
        logf_("Listening on UDP port %d. Type status or quit", g_server.list_port);
        while (!g_quit) {
            console_poll();
            list_server_poll(game_log);
            usleep(10000);
        }
        list_server_stop();
        logf_("Shutting down");
        return 0;
    }
    if (strcmp(g_server.mode, "game") != 0) {
        fprintf(stderr, "mode is game or list, not %s\n", g_server.mode);
        return 1;
    }

    // The engine reads data/... from the game folder, as it does on the PSP
    if (chdir(g_server.game_dir) != 0) {
        fprintf(stderr, "Can't open the game folder %s: %s\n", g_server.game_dir, strerror(errno));
        return 1;
    }
    struct stat st;
    if (stat("data/maps", &st) != 0) {
        fprintf(stderr, "%s has no data/maps: point game_dir at the game's folder (the one with data/ in it)\n",
                g_server.game_dir);
        return 1;
    }
    snprintf(g_settings.name, sizeof(g_settings.name), "%s", g_server.name);

    logf_("Retcon's Mod dedicated server %s (game %s)", SERVER_VERSION, GAME_VERSION);
    mem_init();
    game_preload();
    if (start_map(g_server.map) != 0) return 1;
    if (g_server.build[0]) {
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "loadhere %s", g_server.build);
        game_command(cmd);
    }
    logf_("Type help for commands");

    const SceInt64 step = 1000000 / g_server.tick;
    SceInt64 next = sceKernelGetSystemTimeWide();
    InputState in;
    memset(&in, 0, sizeof(in));
    while (!g_quit) {
        console_poll();
        if (g_in_game) game_update(&in, 1.0f / (float)g_server.tick);
        heartbeat();
        next += step;
        SceInt64 now = sceKernelGetSystemTimeWide();
        if (next > now) {
            usleep((useconds_t)(next - now));
        } else if (now - next > 250000) {
            next = now; // fell well behind (a big map loading): don't race to catch up
        }
    }
    logf_("Shutting down");
    end_map(GAME_LEAVE_ENDED);
    return 0;
}
