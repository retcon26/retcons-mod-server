// The server list (mode list): dedicated servers send it a heartbeat every 30 seconds, and PSPs' Online
// Servers ask it which servers there are. It only hands out addresses: each PSP then asks every server
// about its game itself (PK_INFO), so a list never shows a game that's stopped answering. It needs no game data.

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net.h"
#include "pspkernel.h"
#include "server.h"

#define LIST_EXPIRE_US 90000000 // three missed heartbeats and a server's off the list
#define MAX_LISTED 256

static struct {
    uint8_t addr[6];
    uint8_t version;
    SceInt64 heard;
} g_list[MAX_LISTED];
static int g_sock = -1;

static void addr_text(const uint8_t a[6], char *out, int len) {
    snprintf(out, len, "%d.%d.%d.%d:%d", a[0], a[1], a[2], a[3], (a[4] << 8) | a[5]);
}

int list_server_start(void) {
    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sock < 0) return -1;
    int one = 1;
    setsockopt(g_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    fcntl(g_sock, F_SETFL, fcntl(g_sock, F_GETFL, 0) | O_NONBLOCK);
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)g_server.list_port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(g_sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        close(g_sock);
        g_sock = -1;
        return -1;
    }
    return 0;
}

void list_server_stop(void) {
    if (g_sock >= 0) close(g_sock);
    g_sock = -1;
}

static void heartbeat(const PkHeartbeat *hb, const struct sockaddr_in *from, void (*log)(const char *)) {
    uint8_t a[6];
    memcpy(a, hb->addr, 6);
    if (!a[0] && !a[1] && !a[2] && !a[3]) memcpy(a, &from->sin_addr.s_addr, 4); // no address given: where it came from
    SceInt64 now = sceKernelGetSystemTimeWide();
    int free_slot = -1;
    for (int i = 0; i < MAX_LISTED; i++) {
        if (g_list[i].heard && memcmp(g_list[i].addr, a, 6) == 0) {
            g_list[i].heard = now;
            g_list[i].version = hb->h.version;
            return;
        }
        if (free_slot < 0 && (!g_list[i].heard || now - g_list[i].heard > LIST_EXPIRE_US)) free_slot = i;
    }
    if (free_slot < 0) return; // full
    memcpy(g_list[free_slot].addr, a, 6);
    g_list[free_slot].version = hb->h.version;
    g_list[free_slot].heard = now;
    char at[32], msg[64];
    addr_text(a, at, sizeof(at));
    snprintf(msg, sizeof(msg), "Listed %s (game protocol %d)", at, hb->h.version);
    log(msg);
}

// Answers with the live servers a player of that version can join
static void answer(const NetHeader *req, const struct sockaddr_in *to) {
    PkList l;
    memset(&l, 0, sizeof(l));
    l.h = (NetHeader){NET_MAGIC, PK_LIST, 0, req->version};
    SceInt64 now = sceKernelGetSystemTimeWide();
    for (int i = 0; i < MAX_LISTED && l.count < NET_LIST_MAX; i++) {
        if (!g_list[i].heard || now - g_list[i].heard > LIST_EXPIRE_US || g_list[i].version != req->version) continue;
        memcpy(l.addr[l.count++], g_list[i].addr, 6);
    }
    sendto(g_sock, &l, offsetof(PkList, addr) + (size_t)l.count * 6, 0, (const struct sockaddr *)to, sizeof(*to));
}

void list_server_poll(void (*log)(const char *)) {
    if (g_sock < 0) return;
    for (;;) {
        union {
            NetHeader h;
            PkHeartbeat hb;
            uint8_t raw[512];
        } pk;
        struct sockaddr_in from;
        socklen_t sl = sizeof(from);
        ssize_t len = recvfrom(g_sock, &pk, sizeof(pk), 0, (struct sockaddr *)&from, &sl);
        if (len <= 0) break;
        if (len < (ssize_t)sizeof(NetHeader) || pk.h.magic != NET_MAGIC) continue;
        if (pk.h.type == PK_HEARTBEAT && len >= (ssize_t)sizeof(PkHeartbeat)) {
            heartbeat(&pk.hb, &from, log);
        } else if (pk.h.type == PK_LIST_REQ) {
            answer(&pk.h, &from);
        }
    }
    // Say when a server drops off
    SceInt64 now = sceKernelGetSystemTimeWide();
    for (int i = 0; i < MAX_LISTED; i++) {
        if (g_list[i].heard && now - g_list[i].heard > LIST_EXPIRE_US) {
            char at[32], msg[64];
            addr_text(g_list[i].addr, at, sizeof(at));
            snprintf(msg, sizeof(msg), "Dropped %s (no heartbeat)", at);
            log(msg);
            g_list[i].heard = 0;
        }
    }
}

int list_server_status(void (*log)(const char *)) {
    SceInt64 now = sceKernelGetSystemTimeWide();
    int n = 0;
    for (int i = 0; i < MAX_LISTED; i++) {
        if (!g_list[i].heard) continue;
        char at[32], msg[96];
        addr_text(g_list[i].addr, at, sizeof(at));
        snprintf(msg, sizeof(msg), "  %s  protocol %d  heard %ds ago", at, g_list[i].version,
                 (int)((now - g_list[i].heard) / 1000000));
        log(msg);
        n++;
    }
    return n;
}
