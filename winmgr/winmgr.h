#ifndef _WINMGR_H
#define _WINMGR_H 1

#include <gfx.h>
#include <threads.h>
#include <string.h>
#include <stdio.h>
#include <kora/mcrs.h>
#include <kora/llist.h>


// #define __USE_FT


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
    FNT_ICON,
};

#define BORDER_SHADOW_SIZE 3



typedef struct gfx_win_pack gfx_win_pack_t;
typedef struct window window_t;
typedef struct app app_t;
typedef struct menu menu_t;
typedef struct mitem mitem_t;

struct gfx_win_pack {
    int pid;
    int cmd;
    int opt;
    int shm;
};

struct window {
    int id;
    int pid;
    int shm;
    int x, y;
    int w, h;
    int fpos;
    int cursor;
    gfx_t* win;
    uint32_t color;
    llnode_t node;
    app_t* app;
};

struct app {
    char* name;
    uint32_t color;
    llnode_t node;
    int order;
    window_t* win;
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

    bool (*click)(menu_t*, int);
};

struct mitem {
    llnode_t node;
    uint32_t color;
    char* text;
    void* data;
};


struct winmgr {
    gfx_t* screen;
    gfx_seat_t seat;
    
    mtx_t lock; // BIG FAT LOCK !

    llhead_t win_list;
    llhead_t app_list;

    window_t* win_active;
    window_t* win_over;
    window_t* win_grab;

    int cursorIdx;

    int resizeMode;
    int mouseGrabX;
    int mouseGrabY;
    int grabInitX;
    int grabInitY;
    int grabInitW;
    int grabInitH;

    bool show_menu;

    gfx_t* wallpaper;
    gfx_t* cursors[CRS_MAX];


    menu_t task_menu;
    menu_t start_menu;
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


#define GFX_POINT(x,y)   ((x)|((y)<<16))
#define GFX_POINT_RD(x,y,v)  do { x=(v)&0x7fff; y=((v)>>16)&0x7fff; } while(0)


void gr_shadow(gfx_t* screen, gfx_clip_t* rect, int size, int alpha);
void gr_write(gfx_t* screen, int face, float sizeInPts, const char* str, gfx_clip_t* clip, uint32_t color, int flags);


void menu_paint(gfx_t* screen, menu_t* menu);
bool menu_mouse(menu_t* menu, int x, int y, bool looking);
void menu_button(menu_t* menu, int btn, bool press);
void menu_config(menu_t* menu);

void window_create();
void window_fast_move(window_t* win, int key);
void window_position(window_t* win, gfx_clip_t* rect);
void window_emit(window_t* win, int type, unsigned param);
void window_focus(window_t* win);

void mgr_invalid_screen(int x, int y, int w, int h);
void mgr_paint(gfx_t* screen);
void mgr_mouse_motion(gfx_msg_t* msg);
void mgr_mouse_btn_down(gfx_msg_t* msg);
void mgr_mouse_btn_up(gfx_msg_t* msg);

void config_loading();

#endif /* _WINMGR_H */
