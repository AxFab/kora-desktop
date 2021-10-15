/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#ifndef _SRC_WNS_H
#define _SRC_WNS_H 1

#include <sys/socket.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

typedef struct wns_cnx wns_cnx_t;
typedef struct wns_msg wns_msg_t;

struct wns_cnx {
    SOCKET sock;
    int secret;
    int rlen;
    struct sockaddr remote;
    int status;
};

struct wns_msg {
    int request;
    int secret;
    int winhdl;
    int param;
    int param2;
    int param3;
};

enum {
    WNS_QUIT,
    WNS_HELLO,
    WNS_ACK,
    WNS_NACK,
    WNS_FLIP,
    WNS_OPEN,
    WNS_DESTROY,
    WNS_WINDOW,
    WNS_RESIZE,
    WNS_MOUSE,
    WNS_KEYBOARD,
    WNS_TIMER,
    WNS_TICK,
};

#define WNS_PORT 8765

void wns_initialize();
int wns_connect(wns_cnx_t* cnx);
int wns_send(wns_cnx_t* cnx, wns_msg_t* msg);
int wns_recv(wns_cnx_t* cnx, wns_msg_t* msg);

#define WNS_MSG(m,r,s,w,p,q,t) \
    do { m.request = r; m.secret = s; m.winhdl = w; m.param = p; m.param2 = q; m.param3 = t; } while(0)

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifdef _WIN32
#define WNS_ERROR WSAGetLastError()
static inline void wns_teardown(void)
{
    WSACleanup();
}

static inline void wns_initialize(void)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Failed. Error code : %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    atexit(wns_teardown);
}
#else 
#define WNS_ERROR errno
#endif 

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static inline int wns_connect(wns_cnx_t* cnx)
{
    struct sockaddr_in* server = (struct sockaddr_in*)&cnx->remote;
    server->sin_family = AF_INET;
    server->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server->sin_port = htons(WNS_PORT);

#ifdef _WIN32
    wns_initialize();
#endif 
    cnx->rlen = sizeof(*server);
    cnx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (cnx->sock == INVALID_SOCKET) {
        fprintf(stderr, "Counld not connect to server : %d\n", WNS_ERROR);
        return -1;
    }
    return 0;
}

static inline int wns_send(wns_cnx_t* cnx, wns_msg_t* msg)
{
    int ret = sendto(cnx->sock, (char*)msg, sizeof(wns_msg_t), 0, &cnx->remote, cnx->rlen);
    if (ret == SOCKET_ERROR)
        fprintf(stderr, "sendto() failed with code  : %d\n", WNS_ERROR);
    return ret;
}

static inline int wns_recv(wns_cnx_t* cnx, wns_msg_t* msg)
{
    int rlen = sizeof(struct sockaddr);
    struct sockaddr raddr;
    int ret = recvfrom(cnx->sock, (char*)msg, sizeof(wns_msg_t), 0, &raddr, &rlen);
    if (ret == SOCKET_ERROR)
        fprintf(stderr, "recvfrom() failed with code  : %d\n", WNS_ERROR);
    return ret;
}

#endif  /* _SRC_WNS_H */
