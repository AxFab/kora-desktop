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
#include "winmgr.h"
#include <keycodes.h>
#include <threads.h>

#define RESIZE_MARGE 5

void event_motion(gfx_msg_t *msg)
{
    window_t *win = NULL;

    mgr_invalid_screen(_.seat->mouse_x - _.seat->rel_x - 64, _.seat->mouse_y - _.seat->rel_y - 64, 128, 128);
    mgr_invalid_screen(_.seat->mouse_x - 64, _.seat->mouse_y - 64, 128, 128);

    bool looking = true;
    if (_.show_menu)
        looking = !menu_mouse(&_.start_menu, _.seat->mouse_x, _.seat->mouse_y, looking);
    looking = !menu_mouse(&_.task_menu, _.seat->mouse_x, _.seat->mouse_y, looking);

    if (looking) {
        mtx_lock(&_.lock);
        for ll_each_reverse(&_.win_list, win, window_t, node) {
            gfx_clip_t rect;
            window_position(win, &rect);
            if (rect.left >= _.seat->mouse_x + RESIZE_MARGE)
                continue;
            if (rect.top >= _.seat->mouse_y + RESIZE_MARGE)
                continue;
            if (rect.right < _.seat->mouse_x - RESIZE_MARGE)
                continue;
            if (rect.bottom < _.seat->mouse_y - RESIZE_MARGE)
                continue;

            if (_.win_grab == NULL) {
                if (rect.left >= _.seat->mouse_x) {
                    if (rect.top >= _.seat->mouse_y) {
                        _.cursorIdx = CRS_RESIZE_NW;
                        _.resizeMode = RCT_LEFT | RCT_TOP;

                    } else if (rect.bottom < _.seat->mouse_y) {
                        _.cursorIdx = CRS_RESIZE_SW;
                        _.resizeMode = RCT_LEFT | RCT_BOTTOM;
                    } else {
                        _.cursorIdx = CRS_RESIZE_W;
                        _.resizeMode = RCT_LEFT;
                    }
                } else if (rect.right < _.seat->mouse_x) {
                    if (rect.top >= _.seat->mouse_y) {
                        _.cursorIdx = CRS_RESIZE_NE;
                        _.resizeMode = RCT_RIGHT | RCT_TOP;
                    } else if (rect.bottom < _.seat->mouse_y) {
                        _.cursorIdx = CRS_RESIZE_SE;
                        _.resizeMode = RCT_RIGHT | RCT_BOTTOM;
                    } else {
                        _.cursorIdx = CRS_RESIZE_E;
                        _.resizeMode = RCT_RIGHT;
                    }
                } else if (rect.top >= _.seat->mouse_y) {
                    _.cursorIdx = CRS_RESIZE_N;
                    _.resizeMode = RCT_TOP;
                } else if (rect.bottom < _.seat->mouse_y) {
                    _.cursorIdx = CRS_RESIZE_S;
                    _.resizeMode = RCT_BOTTOM;
                } else {
                    _.cursorIdx = _.win_active != NULL ? _.win_active->cursor : CRS_DEFAULT;
                    _.resizeMode = 0;
                }
            }

            break;
        }

        if (win == NULL && _.win_grab == NULL) {
            _.cursorIdx = _.win_active != NULL ? _.win_active->cursor : CRS_DEFAULT;
            _.resizeMode = 0;
        }

        mtx_unlock(&_.lock);
    }

    if (_.win_grab != NULL) {
        int dispX = _.seat->mouse_x - _.mouseGrabX;
        int dispY = _.seat->mouse_y - _.mouseGrabY;
        mgr_invalid_screen(_.win_grab->x - BORDER_SHADOW_SIZE, _.win_grab->y - BORDER_SHADOW_SIZE, _.win_grab->w + BORDER_SHADOW_SIZE * 2, _.win_grab->h + BORDER_SHADOW_SIZE * 2);

        if (_.resizeMode != 0) {
            if (_.resizeMode & RCT_LEFT) {
                dispX = MIN(dispX, _.grabInitW - 20);
                _.win_grab->x = _.grabInitX + dispX;
                _.win_grab->w = _.grabInitW - dispX;
            } else if (_.resizeMode & RCT_RIGHT) {
                dispX = MAX(dispX, 20 - _.grabInitW);
                _.win_grab->w = _.grabInitW + dispX;
            }

            if (_.resizeMode & RCT_TOP) {
                dispY = MIN(dispY, _.grabInitH - 20);
                _.win_grab->y = _.grabInitY + dispY;
                _.win_grab->h = _.grabInitH - dispY;
            } else if (_.resizeMode & RCT_BOTTOM) {
                dispY = MAX(dispY, 20 - _.grabInitH);
                _.win_grab->h = _.grabInitH + dispY;
            }

            window_emit(_.win_over, GFX_EV_RESIZE, GFX_POINT(_.win_grab->w, _.win_grab->h));
        } else {
            _.win_grab->x = _.grabInitX + dispX;
            _.win_grab->y = _.grabInitY + dispY;
            window_emit(_.win_over, GFX_EV_DISPLACMENT, 0);
        }

        if (_.win_grab->x < 10 - _.win_grab->w)
            _.win_grab->x = 10 - _.win_grab->w;
        if (_.win_grab->y < 10 - _.win_grab->h)
            _.win_grab->y = 10 - _.win_grab->h;
        if (_.win_grab->x > _.screen->width - 10)
            _.win_grab->x = _.screen->width - 10;
        if (_.win_grab->y > _.screen->height - 60)
            _.win_grab->y = _.screen->height - 60;

        mgr_invalid_screen(_.win_grab->x - BORDER_SHADOW_SIZE, _.win_grab->y - BORDER_SHADOW_SIZE, _.win_grab->w + BORDER_SHADOW_SIZE * 2, _.win_grab->h + BORDER_SHADOW_SIZE * 2);
    }
    if (win != _.win_over) {
        // printf("Change of overing window [%p => %p]\n", _.win_over, win);
        window_emit(_.win_over, GFX_EV_EXIT, 0);
        window_emit(win, GFX_EV_ENTER, 0);
        window_emit(_.win_over, GFX_EV_MOUSEMOVE, msg->param1);
    }
    _.win_over = win;
    if (win) {
        int ms = GFX_POINT(_.seat->mouse_x - win->x, _.seat->mouse_y - win->y);
        window_emit(win, GFX_EV_MOUSEMOVE, ms);
    }
}

void event_wheel(gfx_msg_t *msg)
{
    if (_.win_over != NULL)
        window_emit(_.win_over, GFX_EV_MOUSEWHEEL, msg->param1);
    // printf("Mouse wheel (%d)\n", msg.param1);
}

void event_mouse_down(gfx_msg_t *msg)
{
    if (_.show_menu)
        menu_button(&_.start_menu, msg->param1, true);
    menu_button(&_.task_menu, msg->param1, true);
    if (_.win_over != NULL && _.win_over != _.win_active)
        window_focus(_.win_over);
    if (msg->param1 == 1) {
        _.win_grab = _.win_over;
        if (_.win_grab != NULL) {
            _.mouseGrabX = _.seat->mouse_x;
            _.mouseGrabY = _.seat->mouse_y;
            _.grabInitX = _.win_grab->x;
            _.grabInitY = _.win_grab->y;
            _.grabInitW = _.win_grab->w;
            _.grabInitH = _.win_grab->h;
        }
    }

    if (_.win_over)
        window_emit(_.win_over, GFX_EV_BTNDOWN, msg->param1);
}

void event_mouse_up(gfx_msg_t *msg)
{
    bool close_menu = false;
    if (_.show_menu) {
        menu_button(&_.start_menu, msg->param1, false);
        close_menu = true;
        mgr_invalid_screen(0, 0, 0, 0);
    }
    menu_button(&_.task_menu, msg->param1, false);
    if (close_menu)
        _.show_menu = false;

    if (msg->param1 == 1)
        _.win_grab = NULL;

    if (_.win_over)
        window_emit(_.win_over, GFX_EV_BTNUP, msg->param1);
}

void event_keyboard_down(gfx_msg_t *msg)
{
    if (_.seat->kdb_status == KMOD_LCTRL && _.win_active != NULL)
        window_fast_move(_.win_active, msg->param1);
#if 1
    // 16 - Q
    else if (msg->param1 == 30) // A
        window_create(NULL, 200, 200);
    else if (msg->param1 == 31) // S
        _.win_active->cursor = (_.win_active->cursor + 1) % CRS_MAX;
#endif

    if (_.win_active != NULL)
        window_emit(_.win_active, GFX_EV_KEYDOWN, msg->param1);
    // printf("Key down (%d . %o)\n", msg.param1, _.seat->kdb_status);
}

void event_keyboard_up(gfx_msg_t *msg)
{
    if (_.win_active != NULL)
        window_emit(_.win_active, GFX_EV_KEYUP, msg->param1);
    // printf("Key up (%d . %o)\n", msg.param1, _.seat->kdb_status);
}

void event_tick()
{
    wns_msg_t msg;
    cuser_t *usr;
    mtx_lock(&_.lock);
    for ll_each(&_.tmr_list, usr, cuser_t, tnode) {
        WNS_MSG(msg, WNS_TICK, usr->cnx.secret, 0, 0, 0, 0);
        wns_send(&usr->cnx, &msg);
    }
    mtx_unlock(&_.lock);

    // for all
    if (_.invalid) {
        gfx_map(_.screen);
        mgr_paint(_.screen);
        gfx_flip(_.screen, NULL);
        _.invalid = false;
    }
}

void event_loop()
{
    gfx_msg_t msg;
    for (;;) {
        gfx_poll(&msg);
        gfx_handle(&msg);
        switch (msg.message) {
        case GFX_EV_QUIT:
            return;
        case GFX_EV_MOUSEMOVE:
            event_motion(&msg);
            break;
        case GFX_EV_MOUSEWHEEL:
            event_wheel(&msg);
            break;
        case GFX_EV_BTNUP:
            event_mouse_up(&msg);
            break;
        case GFX_EV_BTNDOWN:
            event_mouse_down(&msg);
            break;
        case GFX_EV_KEYDOWN:
            event_keyboard_down(&msg);
            break;
        case GFX_EV_KEYUP:
            event_keyboard_up(&msg);
            break;
        case GFX_EV_TIMER:
            event_tick();
            break;
        }
    }
}
