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
#ifndef _WINMGR_H
#define _WINMGR_H 1

#include <wns.h>
#include <gfx.h>
#include <threads.h>
#include <string.h>
#include <stdio.h>
#include "mcrs.h"
#include "llist.h"
#include "hmap.h"

#define __USE_FT 1


enum {
    GFX_EV_EXIT = 32,
    GFX_EV_ENTER,
    GFX_EV_DISPLACMENT,
};

enum {
    RCT_LEFT = (1 << 0),
    RCT_TOP = (1 << 1),
    RCT_RIGHT = (1 << 2),
    RCT_BOTTOM = (1 << 3),
};

// Mouse cursors
enum {
    CRS_DEFAULT = 0,
    CRS_POINTER,
    CRS_WAIT,
    CRS_MOVE,
    CRS_TEXT,
    CRS_HELP,
    CRS_PROGRESS,
    CRS_NOT_ALLOWED,
    CRS_RESIZE_N,
    CRS_RESIZE_S,
    CRS_RESIZE_E,
    CRS_RESIZE_W,
    CRS_RESIZE_NE,
    CRS_RESIZE_SW,
    CRS_RESIZE_NW,
    CRS_RESIZE_SE,
    CRS_RESIZE_COL,
    CRS_RESIZE_ROW,
    CRS_ALL_SCROLL,
    CRS_CROSSHAIR,
    CRS_DROP,
    CRS_NODROP,
    CRS_MAX
};


// Fonts
enum {
    FNT_DEFAULT,
    FNT_ICON = 4,
};

#define BORDER_SHADOW_SIZE 3



typedef struct gfx_win_pack gfx_win_pack_t;
typedef struct window window_t;
typedef struct app app_t;
typedef struct menu menu_t;
typedef struct mitem mitem_t;
typedef struct cuser cuser_t;

struct gfx_win_pack {
    int pid;
    int cmd;
    int opt;
    int shm;
};

struct window {
    int id;
    //int pid;
    //int shm;
    int x, y;
    int w, h;
    int fpos;
    int cursor;
    gfx_t *gfx;
    uint32_t color;
    llnode_t node;
    llnode_t unode;
    app_t *app;
    cuser_t *usr;
};

struct cuser {
    wns_cnx_t cnx;
    llnode_t tnode;
    llhead_t wins;
};

struct app {
    char *name;
    uint32_t color;
    llnode_t node;
    int order;
    window_t *win;
};

struct menu {
    gfx_clip_t box;
    uint32_t bg_color;
    uint32_t bg1_color;
    uint32_t bg2_color;
    uint32_t bg3_color;
    uint32_t tx_color;
    bool shadow;
    bool vertical;
    int item_size;
    int idx_active;
    int idx_over;
    int idx_grab;
    llhead_t list;
    int pad;
    int gap;
    int font_face;
    float font_size;

    bool (*click)(menu_t *, int);
};

// Menu items (both)
struct mitem {
    llnode_t node;
    uint32_t color;
    char *text;
    void *data;
};


struct winmgr {
    gfx_t *screen;
    gfx_seat_t *seat;

    mtx_t lock; // BIG FAT LOCK !

    llhead_t win_list; // List of opened windows
    llhead_t app_list; // List of connected process
    llhead_t tmr_list; // List of timers

    window_t *win_active;
    window_t *win_over;
    window_t *win_grab;

    int cursorIdx;

    int resizeMode;
    int mouseGrabX;
    int mouseGrabY;
    int grabInitX;
    int grabInitY;
    int grabInitW;
    int grabInitH;
    bool invalid;

    bool show_menu;
    bool show_cursor;
    bool show_taskbar;

    gfx_t *wallpaper;
    gfx_t *cursors[CRS_MAX];

    gfx_font_t *fontFaces[16];

    menu_t task_menu;
    menu_t start_menu;

    mtx_t _cnx_lk;
    hmap_t _cnx;
};

extern struct winmgr _;


#define WSZ_NORMAL 0
#define WSZ_MAXIMIZED 1
#define WSZ_LEFT 2
#define WSZ_RIGHT 3
#define WSZ_TOP 4
#define WSZ_BOTTOM 5
#define WSZ_ANGLE_TL 6
#define WSZ_ANGLE_TR 7
#define WSZ_ANGLE_BL 8
#define WSZ_ANGLE_BR 9



void gr_shadow(gfx_t *screen, gfx_clip_t *rect, int size, int alpha);
void gr_write(gfx_t *screen, int face, float sizeInPts, const char *str, gfx_clip_t *clip, uint32_t color, int flags);


void menu_paint(gfx_t *screen, menu_t *menu);
bool menu_mouse(menu_t *menu, int x, int y, bool looking);
void menu_button(menu_t *menu, int btn, bool press);
void menu_config(menu_t *menu);

window_t *window_create(cuser_t *usr, int width, int height);
void window_fast_move(window_t *win, int key);
void window_position(window_t *win, gfx_clip_t *rect);
void window_emit(window_t *win, int type, unsigned param);
void window_focus(window_t *win);

void mgr_invalid_screen(int x, int y, int w, int h);
void mgr_paint(gfx_t *screen);

void event_loop();
void wns_service();
void resx_loading();

void screens_loading();

#define RESX "H:/kora/sources/desktop/resx"


#endif /* _WINMGR_H */
