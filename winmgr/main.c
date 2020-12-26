#include <keycodes.h>
#include "winmgr.h"

struct winmgr _;

//void service(void* arg) {
//
//    int sock = -1;
//    struct sockaddr_in addr;
//    addr.sin_family = AF_INET;
//    addr.sin_addr.s_addr = INADDR_ANY;
//    addr.sin_port = htons(1842);
//
//    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//    bind(sock, &addr, sizeof(addr));
//    listen(sock, 5);
//
//    for (;;) {
//        gfx_win_pack_t pack;
//        recv(sock, &pack, sizeof(pack), 0);
//
//        printf("Command received\n");
//        mtx_lock(&BFL);
//
//        struct window* win = malloc(sizeof(struct window));
//        win->pid = pack.pid;
//        win->shm = pack.shm;
//        int fd = // OPEN SHM !!
//        // win->win = gfx_opend();
//
//        mtx_unlock(&BFL);
//    }
//
//}

int main()
{
    memset(&_, 0, sizeof(_));
    config_loading();

    gfx_msg_t msg;
    for (;;) {
        gfx_poll(_.screen, &msg);
        gfx_handle(_.screen, &msg, &_.seat);
        if (msg.message == GFX_EV_QUIT)
            break;
        else if (msg.message == GFX_EV_PAINT) {
            gfx_map(_.screen);
            mgr_paint(_.screen);
            gfx_flip(_.screen);
            continue;
        } else if (msg.message == GFX_EV_TIMER)
            continue;


        switch (msg.message) {

        case GFX_EV_MOUSEMOVE:
            mgr_mouse_motion(&msg);
            break;

        case GFX_EV_MOUSEWHEEL:
            printf("Mouse wheel (%d)\n", msg.param1);
            break;

        case GFX_EV_BTNUP:
            mgr_mouse_btn_up(&msg);
            break;

        case GFX_EV_BTNDOWN:
            mgr_mouse_btn_down(&msg);
            break;

        case GFX_EV_KEYDOWN:
            // 16 - Q
            if (msg.param1 == 30) // A
                window_create();
            else if (msg.param1 == 31)
                _.win_active->cursor = (_.win_active->cursor + 1) % CRS_MAX;
            else if (_.seat.kdb_status == KMOD_LCTRL && _.win_active != NULL)
                window_fast_move(_.win_active, msg.param1);
            printf("Key down (%d . %o)\n", msg.param1, _.seat.kdb_status);
            break;

        case GFX_EV_KEYUP:
            printf("Key up (%d . %o)\n", msg.param1, _.seat.kdb_status);
            break;
        }
    }

    gfx_destroy(_.screen);
    return 0;
}
