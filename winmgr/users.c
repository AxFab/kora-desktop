/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
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
#include "wns.h"
#include "winmgr.h"

static cuser_t *wns_fetch_session(SOCKET sock, void *key, int len, int secret)
{
    cuser_t *usr;
    mtx_lock(&_._cnx_lk);
    usr = hmp_get(&_._cnx, key, len);
    if (usr == NULL) {
        if (secret != 0)
            goto done;

        usr = calloc(sizeof(cuser_t), 1);
        if (usr == NULL) {
            fprintf(stderr, "Server is out of memory\n");
            goto done;
        }

        do {
            usr->cnx.secret = rand() | (rand() << 16);
        } while (usr->cnx.secret == 0);

        usr->cnx.sock = sock;
        usr->cnx.rlen = len;
        memcpy(&usr->cnx.remote, key, len);
        hmp_put(&_._cnx, key, len, usr);

    } else if (secret == 0 || usr->cnx.secret != secret)
        usr = NULL;
done:
    mtx_unlock(&_._cnx_lk);
    return usr;
}

static window_t *wns_window(cuser_t *usr, int uid)
{
    window_t *win;
    for ll_each(&usr->wins, win, window_t, unode) {
        if (win->id == uid)
            return win;
    }
    return NULL;
}

static void wns_handler(cuser_t *usr, wns_msg_t *msg, wns_msg_t *res)
{
    window_t *win;
    switch (msg->request) {
    case WNS_OPEN:
        win = window_create(usr, msg->param, msg->param2);
        res->request = WNS_WINDOW;
        res->winhdl = win->id;
        res->param = msg->param;
        res->param2 = msg->param2;
        ll_append(&usr->wins, &win->unode);
        break;
    case WNS_HELLO:
        res->request = WNS_ACK;
        break;
    case WNS_TIMER:
        res->request = WNS_ACK;

        mtx_lock(&_.lock);
        ll_append(&_.tmr_list, &usr->tnode);
        mtx_unlock(&_.lock);
        // TODO -- Allocate timer ID !
        break;
    case WNS_FLIP:
        mtx_lock(&_.lock);
        win = wns_window(usr, msg->winhdl);
        if (win) {
            gfx_flip(win->gfx, (void *)msg->param3);
            gfx_clip_t rect;
            window_position(win, &rect);
            mgr_invalid_screen(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
        }
        mtx_unlock(&_.lock);
        break;
    case WNS_RESIZE:
        mtx_lock(&_.lock);
        win = wns_window(usr, msg->winhdl);
        if (win) {
            gfx_unmap(win->gfx);
            gfx_resize(win->gfx, msg->param, msg->param2);
            gfx_map(win->gfx);
        }
        mtx_unlock(&_.lock);
        break;
    default:
        break;
    }
}

void wns_service()
{
    SOCKET sock;
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server.sin_port = htons(WNS_PORT);

    hmp_init(&_._cnx, 16);

#ifdef _WIN32
    wns_initialize();
#endif
    mtx_init(&_._cnx_lk, mtx_plain);
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "Could not create socket : %d\n", WNS_ERROR);
        return;
    }

    int ret = bind(sock, (void *)&server, sizeof(server));
    if (ret == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed with error code : %d\n", WNS_ERROR);
        return;
    }

    printf("WNS Service is up\n");
    for (;;) {
        wns_msg_t msg;
        wns_msg_t res;
        struct sockaddr_in remote;
        int rlen = sizeof(remote);
        // printf("Wait cnx\n");
        int ret = recvfrom(sock, (char *)&msg, sizeof(msg), 0, (struct sockaddr *)&remote, &rlen);
        // printf("Get cnx %d\n", ret);
        if (ret == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom() failed with error code : %d\n", WNS_ERROR);
            return;
        }

        res.request = -1;
        cuser_t *usr = wns_fetch_session(sock, &remote, rlen, msg.request == WNS_HELLO ? 0 : msg.secret);
        if (usr != NULL) {
            wns_handler(usr, &msg, &res);
            res.secret = usr->cnx.secret;
            if (res.request != -1)
                wns_send(&usr->cnx, &res);
        }
    }

    closesocket(sock);
}
