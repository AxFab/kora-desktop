#include "winmgr.h"

int win_ID = 0;

int fast_move[][4] = {
    // L R U D
    { WSZ_LEFT, WSZ_RIGHT, WSZ_TOP, WSZ_BOTTOM },
    { WSZ_LEFT, WSZ_RIGHT, WSZ_TOP, WSZ_BOTTOM },
    { WSZ_NORMAL, WSZ_NORMAL, WSZ_ANGLE_TL, WSZ_ANGLE_BL }, // Left
    { WSZ_NORMAL, WSZ_NORMAL, WSZ_ANGLE_TR, WSZ_ANGLE_BR }, // Right
    { WSZ_ANGLE_TL, WSZ_ANGLE_TR, WSZ_MAXIMIZED, WSZ_NORMAL }, // Top
    { WSZ_ANGLE_BL, WSZ_ANGLE_BR, WSZ_NORMAL, WSZ_NORMAL }, // Bottom
    { WSZ_LEFT, WSZ_ANGLE_TR, WSZ_TOP, WSZ_ANGLE_BL }, // TL
    { WSZ_ANGLE_TL, WSZ_RIGHT, WSZ_TOP, WSZ_ANGLE_BR }, // TR
    { WSZ_LEFT, WSZ_ANGLE_BR, WSZ_ANGLE_TL, WSZ_BOTTOM }, // BL
    { WSZ_ANGLE_BL, WSZ_RIGHT, WSZ_ANGLE_TR, WSZ_BOTTOM }, // BR
};

const char *eventName[] = {
    "0",
    "Mouse-motion",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "Window-resize",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
    "17",
    "18",
    "19",
    "20",
    "21",
    "22",
    "23",
    "24",
    "25",
    "26",
    "27",
    "28",
    "29",
    "30",
    "31",
    "Mouse-exit",
    "Mouse-enter",
    "Window-displacement",
    "35",
    "36",
    "37",
    "38",
    "39",
    "40",
    "41",
    "42",
    "43",
    "44",
    "45",
    "46",
    "47",
    "48",
    "49",
};


void window_create()
{
    window_t *win = malloc(sizeof(window_t));
    win->id = ++win_ID;
    win->pid = 0;
    win->shm = 0;
    win->x = 0;
    win->y = 0;
    win->w = 200;
    win->h = 200;
    win->fpos = 0;
    win->win = gfx_create_surface(200, 200);
    win->color = (rand() ^ (rand() << 16)) & 0xFFFFFF;
    win->cursor = CRS_DEFAULT;
    gfx_map(win->win);
    gfx_fill(win->win, win->color, GFX_NOBLEND, NULL);

    app_t *app = malloc(sizeof(app_t));
    app->color = win->color;
    win->app = app;
    app->win = win; // TODO -- Multiple window per application !?

    mtx_lock(&_.lock);
    ll_append(&_.win_list, &win->node);
    ll_append(&_.app_list, &app->node);


    mitem_t *item = malloc(sizeof(mitem_t));
    item->color = win->color;
    item->text = NULL;
    item->data = app;
    ll_append(&_.task_menu.list, &item->node);


    app->order = _.app_list.count_;

    mtx_unlock(&_.lock);
    window_focus(win);
}

void window_fast_move(window_t *win, int key)
{
    gfx_t *screen = _.screen;
    int pos = -1;
    if (key == 75) {
        printf("Move to left\n");
        pos = fast_move[win->fpos][0];
    } else if (key == 77) {
        printf("Move to right\n");
        pos = fast_move[win->fpos][1];
    } else if (key == 72)
        pos = fast_move[win->fpos][2];
    else if (key == 80)
        pos = fast_move[win->fpos][3];

    if (pos < 0)
        return;

    win->fpos = pos;
    gfx_invalid(screen);
    switch (win->fpos) {
    case WSZ_MAXIMIZED:
        gfx_resize(win->win, screen->width, screen->height - 50);
        break;
    case WSZ_LEFT:
    case WSZ_RIGHT:
        gfx_resize(win->win, screen->width / 2, screen->height - 50);
        break;
    case WSZ_TOP:
    case WSZ_BOTTOM:
        gfx_resize(win->win, screen->width, (screen->height - 50) / 2);
        break;
    case WSZ_ANGLE_TL:
    case WSZ_ANGLE_TR:
    case WSZ_ANGLE_BL:
    case WSZ_ANGLE_BR:
        gfx_resize(win->win, screen->width / 2, (screen->height - 50) / 2);
        break;
    case WSZ_NORMAL:
    default:
        gfx_resize(win->win, win->w, win->h);
        break;
    }

    window_emit(win, GFX_EV_RESIZE, GFX_POINT(win->win->width, win->win->height));
    gfx_map(win->win);
    gfx_fill(win->win, win->color, GFX_NOBLEND, NULL);
}

void window_position(window_t *win, gfx_clip_t *rect)
{
    gfx_t *screen = _.screen;
    switch (win->fpos) {
    case WSZ_MAXIMIZED:
        rect->left = 0;
        rect->right = screen->width;
        rect->top = 0;
        rect->bottom = screen->height - 50;
        break;
    case WSZ_LEFT:
        rect->left = 0;
        rect->right = screen->width / 2;
        rect->top = 0;
        rect->bottom = screen->height - 50;
        break;
    case WSZ_RIGHT:
        rect->left = screen->width / 2;
        rect->right = screen->width;
        rect->top = 0;
        rect->bottom = screen->height - 50;
        break;
    case WSZ_TOP:
        rect->left = 0;
        rect->right = screen->width;
        rect->top = 0;
        rect->bottom = (screen->height - 50) / 2;
        break;
    case WSZ_BOTTOM:
        rect->left = 0;
        rect->right = screen->width;
        rect->top = (screen->height - 50) / 2;
        rect->bottom = screen->height - 50;
        break;
    case WSZ_ANGLE_TL:
        rect->left = 0;
        rect->right = screen->width / 2;
        rect->top = 0;
        rect->bottom = (screen->height - 50) / 2;
        break;
    case WSZ_ANGLE_TR:
        rect->left = screen->width / 2;
        rect->right = screen->width;
        rect->top = 0;
        rect->bottom = (screen->height - 50) / 2;
        break;
    case WSZ_ANGLE_BL:
        rect->left = 0;
        rect->right = screen->width / 2;
        rect->top = (screen->height - 50) / 2;
        rect->bottom = screen->height - 50;
        break;
    case WSZ_ANGLE_BR:
        rect->left = screen->width / 2;
        rect->right = screen->width;
        rect->top = (screen->height - 50) / 2;
        rect->bottom = screen->height - 50;
        break;
    case WSZ_NORMAL:
    default:
        rect->left = win->x;
        rect->right = win->x + win->w;
        rect->top = win->y;
        rect->bottom = win->y + win->h;
        break;
    }
}

void window_emit(window_t *win, int type, unsigned param)
{
    if (win == NULL)
        return;
    if (strlen(eventName[type]) > 3)
        printf("Gfx event [%p] %s (%x)\n", win, type < 50 ? eventName[type] : "Unknown", param);
    else
        printf("Gfx event [%p] %s (%x)\n", win, type < 50 ? eventName[type] : "Unknown", param);
}

void window_focus(window_t *win)
{
    mtx_lock(&_.lock);
    ll_remove(&_.win_list, &win->node);
    _.win_active = win;
    ll_append(&_.win_list, &win->node);
    // UPDATE !?
    _.task_menu.idx_active = win->app->order;
    mtx_unlock(&_.lock);
    gfx_invalid(_.screen);
}

