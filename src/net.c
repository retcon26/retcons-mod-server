// The engine's network layer (net.h) for the server: UDP on the game port, the same packets the PSPs
// send over Wi-Fi. Addresses are an IPv4 address and port packed into the engine's 6-byte "MAC".
// "Send to everyone" goes to every address heard from lately (the players), and beacons go round the
// LAN too so Online Servers finds the server without typing its address.

#include "net.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "pspkernel.h"
#include "server.h"

#define PEER_EXPIRE_US 10000000
#define MAX_PEERS 16

static struct {
    NetState state;
    int sock;
    uint8_t mac[6];
    char status[64];
    struct {
        uint8_t addr[6];
        SceInt64 heard;
    } peers[MAX_PEERS];
} N = {.sock = -1};

int net_active(void) {
    return N.state == NET_READY;
}

const char *net_status(void) {
    return N.status;
}

const uint8_t *net_my_mac(void) {
    return N.mac;
}

const char *net_nickname(void) {
    return g_server.name;
}

void net_set_mode(int mode, int host) {
    (void)mode;
    (void)host;
}

int net_mode(void) {
    return NET_MODE_INET;
}

static void addr_pack(uint8_t out[6], uint32_t ip_net, uint16_t port) {
    memcpy(out, &ip_net, 4);
    out[4] = (uint8_t)(port >> 8);
    out[5] = (uint8_t)(port & 0xFF);
}

static void addr_sockaddr(const uint8_t a[6], struct sockaddr_in *sa) {
    memset(sa, 0, sizeof(*sa));
    sa->sin_family = AF_INET;
    memcpy(&sa->sin_addr.s_addr, a, 4);
    sa->sin_port = htons((uint16_t)((a[4] << 8) | a[5]));
}

void net_addr_text(const uint8_t a[6], char *out, int len) {
    snprintf(out, len, "%d.%d.%d.%d:%d", a[0], a[1], a[2], a[3], (a[4] << 8) | a[5]);
}

int net_resolve(const char *text, uint8_t addr[6]) {
    char host[64];
    snprintf(host, sizeof(host), "%s", text);
    int port = NET_PORT;
    char *colon = strrchr(host, ':');
    if (colon) {
        *colon = 0;
        port = atoi(colon + 1);
    }
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0) return -1;
    addr_pack(addr, ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr, (uint16_t)port);
    freeaddrinfo(res);
    return 0;
}

int net_start(void) {
    if (N.state == NET_READY) return 0;
    memset(N.peers, 0, sizeof(N.peers));
    N.sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (N.sock < 0) {
        snprintf(N.status, sizeof(N.status), "Couldn't open a socket");
        N.state = NET_FAILED;
        return -1;
    }
    int one = 1;
    setsockopt(N.sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
    setsockopt(N.sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    fcntl(N.sock, F_SETFL, fcntl(N.sock, F_GETFL, 0) | O_NONBLOCK);
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)g_server.port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(N.sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        snprintf(N.status, sizeof(N.status), "Port %d is in use", g_server.port);
        close(N.sock);
        N.sock = -1;
        N.state = NET_FAILED;
        return -1;
    }
    addr_pack(N.mac, 0, (uint16_t)g_server.port);
    snprintf(N.status, sizeof(N.status), "Listening on UDP port %d", g_server.port);
    N.state = NET_READY;
    return 0;
}

NetState net_poll(void) {
    return N.state;
}

void net_stop(void) {
    if (N.sock >= 0) close(N.sock);
    N.sock = -1;
    N.state = NET_OFF;
    snprintf(N.status, sizeof(N.status), "Offline");
}

static void send_to(const uint8_t a[6], const void *data, int len) {
    struct sockaddr_in sa;
    addr_sockaddr(a, &sa);
    sendto(N.sock, data, (size_t)len, 0, (struct sockaddr *)&sa, sizeof(sa));
}

int net_send(const uint8_t *mac, const void *data, int len) {
    if (N.sock < 0) return -1;
    if (mac) {
        send_to(mac, data, len);
        return len;
    }
    if (((const NetHeader *)data)->type == PK_BEACON && g_server.lan) {
        uint8_t lan[6];
        addr_pack(lan, htonl(INADDR_BROADCAST), NET_PORT);
        send_to(lan, data, len);
    }
    SceInt64 now = sceKernelGetSystemTimeWide();
    for (int i = 0; i < MAX_PEERS; i++) {
        if (N.peers[i].heard && now - N.peers[i].heard < PEER_EXPIRE_US) send_to(N.peers[i].addr, data, len);
    }
    return len;
}

static void peer_heard(const uint8_t a[6]) {
    SceInt64 now = sceKernelGetSystemTimeWide();
    int oldest = 0;
    for (int i = 0; i < MAX_PEERS; i++) {
        if (memcmp(N.peers[i].addr, a, 6) == 0) {
            N.peers[i].heard = now;
            return;
        }
        if (N.peers[i].heard < N.peers[oldest].heard) oldest = i;
    }
    memcpy(N.peers[oldest].addr, a, 6);
    N.peers[oldest].heard = now;
}

int net_recv(uint8_t mac[6], void *buf, int max) {
    if (N.sock < 0) return 0;
    for (;;) {
        struct sockaddr_in sa;
        socklen_t sl = sizeof(sa);
        ssize_t len = recvfrom(N.sock, buf, (size_t)max, 0, (struct sockaddr *)&sa, &sl);
        if (len <= 0) return 0;
        addr_pack(mac, sa.sin_addr.s_addr, ntohs(sa.sin_port));
        const NetHeader *h = (const NetHeader *)buf;
        if (getenv("PSB_DEBUG")) {
            char at[32];
            net_addr_text(mac, at, sizeof(at));
            fprintf(stderr, "recv %d bytes from %s: %02X %02X %02X %02X\n", (int)len, at, ((uint8_t *)buf)[0],
                    ((uint8_t *)buf)[1], ((uint8_t *)buf)[2], ((uint8_t *)buf)[3]);
        }
        if (len < (ssize_t)sizeof(NetHeader) || h->magic != NET_MAGIC) continue;
        if (h->type == PK_BEACON) continue; // another game's beacon (or our own, back off the LAN)
        if (h->type != PK_INFO) peer_heard(mac);
        return (int)len;
    }
}

// The lobby is the PSP's Join screen: a server never joins anything
void net_lobby_query(const uint8_t *addr) {
    (void)addr;
}
void net_lobby_join(const NetHost *h) {
    (void)h;
}
void net_lobby_result(char *map, int map_len, int *slot, uint8_t host_mac[6]) {
    (void)map;
    (void)map_len;
    (void)slot;
    (void)host_mac;
}
int net_lobby_update(float dt, NetHost *hosts, int max_hosts, int *num_hosts) {
    (void)dt;
    (void)hosts;
    (void)max_hosts;
    *num_hosts = 0;
    return 0;
}
