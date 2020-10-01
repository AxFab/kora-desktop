#include <keycodes.h>
#include <time.h>
#include "winmgr.h"

#define RESIZE_MARGE 5

const char* month[] = {
    "JAN", "FEV", "MAR", "APR",
    "MAY", "JUN", "JUL", "AUG",
    "SEP", "OCT", "NOV", "DEC"
};

void mgr_invalid_screen(int x, int y, int w, int h)
{
    // TODO -- Keep track of what to redraw !
    gfx_invalid(_.screen);
}

void mgr_paint_clock(gfx_t* screen)
{
    char tmp[12];
    time_t now = time(NULL);
    struct tm nowTm;
#ifndef main
    localtime_s(&nowTm, &now);
#else
    gmtime_r(&nowTm, &now);
#endif

    // Print Clock
    gfx_clip_t dtDay;
    dtDay.left = screen->width - 42;
    dtDay.right = screen->width - 10;
    dtDay.top = screen->height - 50;
    dtDay.bottom = screen->height;
    snprintf(tmp, 12, "%02d", nowTm.tm_mday);
    gr_write(screen, FNT_DEFAULT, 20, tmp, &dtDay, 0xffffff, 0);

    gfx_clip_t dtMon;
    dtMon.left = screen->width - 74;
    dtMon.right = screen->width - 42;
    dtMon.top = screen->height - 40;
    dtMon.bottom = screen->height - 25;
    gr_write(screen, FNT_DEFAULT, 10, month[nowTm.tm_mon], &dtMon, 0xffffff, 0);

    gfx_clip_t dtYear;
    dtYear.left = screen->width - 74;
    dtYear.right = screen->width - 42;
    dtYear.top = screen->height - 25;
    dtYear.bottom = screen->height - 10;
    snprintf(tmp, 12, "%02d", nowTm.tm_year + 1900);
    gr_write(screen, FNT_DEFAULT, 10, tmp, &dtYear, 0xffffff, 0);

    gfx_clip_t clkClip;
    clkClip.left = screen->width - 140;
    clkClip.right = screen->width - 100;
    clkClip.bottom = screen->height;
    clkClip.top = screen->height - 50;
    snprintf(tmp, 12, "%02d:%02d", nowTm.tm_hour, nowTm.tm_min);
    gr_write(screen, FNT_DEFAULT, 20, tmp, &clkClip, 0xffffff, 0);
}

void mgr_paint(gfx_t *screen)
{
    // Background color / wallpaper
    if (_.wallpaper == NULL)
        gfx_fill(screen, 0xf3e4c6, GFX_NOBLEND, NULL);
    else
        gfx_blit(screen, _.wallpaper, GFX_NOBLEND, NULL, NULL);

    // Windows
    mtx_lock(&_.lock);
    window_t *win;
    for ll_each(&_.win_list, win, window_t, node) {
        gfx_clip_t winClip;
        window_position(win, &winClip);
        gfx_map(win->win);
        gfx_blit(screen, win->win, GFX_NOBLEND, &winClip, NULL);
        gr_shadow(screen, &winClip, BORDER_SHADOW_SIZE, 140);
    }
    mtx_unlock(&_.lock);

    if (_.show_menu) {
        // Start Menu 
        menu_paint(screen, &_.start_menu);
    }

    // Task bar
    menu_paint(screen, &_.task_menu);
    // mgr_paint_clock(screen);

    gfx_clip_t clkClip;
    clkClip.left = 0;
    clkClip.right = 50;
    clkClip.bottom = screen->height;
    clkClip.top = screen->height - 50;
    gr_write(screen, FNT_ICON, 20, "\xef\x80\x82", &clkClip, 0xffffff, 0);


    // Mouse
    gfx_clip_t mseClip;
    gfx_t *cursor = _.cursors[_.cursorIdx];
    if (cursor == NULL)
        cursor = _.cursors[0];
    mseClip.left = _.seat.mouse_x - cursor->width / 2;
    mseClip.right = _.seat.mouse_x + cursor->width / 2;
    mseClip.bottom = _.seat.mouse_y + cursor->height / 2;
    mseClip.top = _.seat.mouse_y - cursor->height / 2;
    gfx_blit(screen, cursor, GFX_CLRBLEND, &mseClip, NULL);
}

void mgr_mouse_motion(gfx_msg_t *msg) 
{
    window_t* win = NULL;

    mgr_invalid_screen(_.seat.mouse_x - _.seat.rel_x - 64, _.seat.mouse_y - _.seat.rel_y - 64, 128, 128);
    mgr_invalid_screen(_.seat.mouse_x - 64, _.seat.mouse_y - 64, 128, 128);

    bool looking = true;
    if (_.show_menu)
        looking = !menu_mouse(&_.start_menu, _.seat.mouse_x, _.seat.mouse_y, looking);
    looking = !menu_mouse(&_.task_menu, _.seat.mouse_x, _.seat.mouse_y, looking);

    if (looking) {
        mtx_lock(&_.lock);
        for ll_each_reverse(&_.win_list, win, window_t, node) {
            gfx_clip_t rect;
            window_position(win, &rect);
            if (rect.left >= _.seat.mouse_x + RESIZE_MARGE)
                continue;
            if (rect.top >= _.seat.mouse_y + RESIZE_MARGE)
                continue;
            if (rect.right < _.seat.mouse_x - RESIZE_MARGE)
                continue;
            if (rect.bottom < _.seat.mouse_y - RESIZE_MARGE)
                continue;

            if (_.win_grab == NULL) {
                if (rect.left >= _.seat.mouse_x) {
                    if (rect.top >= _.seat.mouse_y) {
                        _.cursorIdx = CRS_RESIZE_NW;
                        _.resizeMode = RCT_LEFT | RCT_TOP;

                    }
                    else if (rect.bottom < _.seat.mouse_y) {
                        _.cursorIdx = CRS_RESIZE_SW;
                        _.resizeMode = RCT_LEFT | RCT_BOTTOM;
                    }
                    else {
                        _.cursorIdx = CRS_RESIZE_W;
                        _.resizeMode = RCT_LEFT;
                    }
                }
                else if (rect.right < _.seat.mouse_x) {
                    if (rect.top >= _.seat.mouse_y) {
                        _.cursorIdx = CRS_RESIZE_NE;
                        _.resizeMode = RCT_RIGHT | RCT_TOP;
                    }
                    else if (rect.bottom < _.seat.mouse_y) {
                        _.cursorIdx = CRS_RESIZE_SE;
                        _.resizeMode = RCT_RIGHT | RCT_BOTTOM;
                    }
                    else {
                        _.cursorIdx = CRS_RESIZE_E;
                        _.resizeMode = RCT_RIGHT;
                    }
                }
                else if (rect.top >= _.seat.mouse_y) {
                    _.cursorIdx = CRS_RESIZE_N;
                    _.resizeMode = RCT_TOP;
                }
                else if (rect.bottom < _.seat.mouse_y) {
                    _.cursorIdx = CRS_RESIZE_S;
                    _.resizeMode = RCT_BOTTOM;
                }
                else {
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
        int dispX = _.seat.mouse_x - _.mouseGrabX;
        int dispY = _.seat.mouse_y - _.mouseGrabY;
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

            gfx_unmap(_.win_grab->win);
            gfx_resize(_.win_grab->win, _.win_grab->w, _.win_grab->h);
            gfx_map(_.win_grab->win);
            gfx_fill(_.win_grab->win, _.win_grab->color, GFX_NOBLEND, NULL);
            window_emit(_.win_over, GFX_EV_RESIZE, GFX_POINT(_.win_grab->w, _.win_grab->h));
        }
        else {
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
    window_emit(win, GFX_EV_MOUSEMOVE, msg->param1);
}

void mgr_mouse_btn_down(gfx_msg_t* msg)
{
    if (_.show_menu)
        menu_button(&_.start_menu, msg->param1, true);
    menu_button(&_.task_menu, msg->param1, true);
    if (_.win_over != NULL && _.win_over != _.win_active) {
        window_focus(_.win_over);
    }
    if (msg->param1 == 1) {
        _.win_grab = _.win_over;
        if (_.win_grab != NULL) {
            _.mouseGrabX = _.seat.mouse_x;
            _.mouseGrabY = _.seat.mouse_y;
            _.grabInitX = _.win_grab->x;
            _.grabInitY = _.win_grab->y;
            _.grabInitW = _.win_grab->w;
            _.grabInitH = _.win_grab->h;
        }
    }
    // printf("Mouse btn down (%d)\n", msg.param1);
}

void mgr_mouse_btn_up(gfx_msg_t * msg)
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

    if (msg->param1 == 1) {
        _.win_grab = NULL;
    }
    // printf("Mouse btn up (%d)\n", msg.param1);

}
