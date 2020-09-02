#include <gfx.h>
#include <string.h>
#include <math.h>
#include <threads.h>
#include <stdio.h>
#include <kora/mcrs.h>
#include <kora/llist.h>
#include <keycodes.h>

#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct gfx_win_pack gfx_win_pack_t;
typedef struct window window_t;

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
    gfx_t* win;
    uint32_t color;
    llnode_t node;
};

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
mtx_t BFL;
llhead_t win_list = INIT_LLHEAD;
gfx_t* screen;
gfx_t* wallpaper;
int win_ID = 0;
window_t* grab = NULL;
gfx_seat_t seat;
gfx_t* cursors[CRS_MAX];
int cursorIdx = CRS_DEFAULT;
window_t* active = NULL;

void set_shadow_corner(gfx_t* screen, int size, int sx, int ex, int sy, int ey, int mx, int my, int max)
{
    for (int i = sy; i < ey; ++i)
        for (int j = sx; j < ex; ++j) {
            int dm = abs(j - mx) + abs(i - my);
            float l = abs(j - mx) * abs(j - mx) + abs(i - my) * abs(i - my);
            float l2 = sqrt(l);
            float d = l2 / (float)(size);
            if (d < 1.0f) {
                int a = (1 - d) * (float)max;
                screen->pixels4[i * screen->width + j] = gfx_upper_alpha_blend(screen->pixels4[i * screen->width + j], a << 24);
            }
        }
}

void set_shadow(gfx_t * screen, gfx_clip_t * rect, int size, int max)
{
    int* grad = malloc((size + 1) * sizeof(int));
    for (int i = 0; i < size + 1; ++i)
        grad[i] = (1.0 - sqrt(i / (float)size)) * (float)max;

    // TOP LEFT CORNER
    int miny = MAX(0, rect->top - size);
    int maxy = MAX(0, rect->top);
    int minx = MAX(0, rect->left - size);
    int maxx = MAX(0, rect->left);
    set_shadow_corner(screen, size, minx, maxx, miny, maxy, maxx, maxy, max);


    int minx2 = MIN(rect->right, screen->width);
    int maxx2 = MIN(rect->right + size, screen->width);

    int miny2 = MIN(rect->bottom, screen->height);
    int maxy2 = MIN(rect->bottom + size, screen->height);

    for (int i = miny; i < maxy; ++i)
        for (int j = maxx; j < minx2; ++j) {
            int a = grad[maxy - i - 1];
            screen->pixels4[i * screen->width + j] = gfx_upper_alpha_blend(screen->pixels4[i * screen->width + j], a << 24);
        }


    for (int i = miny2; i < maxy2; ++i)
        for (int j = maxx; j < minx2; ++j) {
            int a = grad[i - miny2];
            screen->pixels4[i * screen->width + j] = gfx_upper_alpha_blend(screen->pixels4[i * screen->width + j], a << 24);
        }

    for (int i = maxy; i < miny2; ++i) {
        for (int j = minx; j < maxx; ++j) {
            int a = grad[maxx - j - 1];
            screen->pixels4[i * screen->width + j] = gfx_upper_alpha_blend(screen->pixels4[i * screen->width + j], a << 24);
        }
        for (int j = minx2; j < maxx2; ++j) {
            int a = grad[j - minx2];
            screen->pixels4[i * screen->width + j] = gfx_upper_alpha_blend(screen->pixels4[i * screen->width + j], a << 24);
        }
    }


    set_shadow_corner(screen, size, minx2, maxx2, miny, maxy, minx2, maxy, max);
    set_shadow_corner(screen, size, minx, maxx, miny2, maxy2, maxx, miny2, max);
    set_shadow_corner(screen, size, minx2, maxx2, miny2, maxy2, minx2, miny2, max);

    free(grad);
}

FT_Library  library;   /* handle to library     */
FT_Face     face;      /* handle to face object */
FT_Error fterror;
FT_Face baseFont;
FT_Face iconFont;


int unichar(const char** txt)
{
    const char* str = *txt;
    int charcode = ((unsigned char*)str)[0];
    str++;

    if (charcode & 0x80) { // mbstring !
        int lg = 1;
        if (charcode >= 0xFF) {
            charcode = 0; // 8
            lg = 0; // 7 x 6 => 42bits
        }
        else if (charcode >= 0xFE) {
            charcode = 0; // 7
            lg = 0; // 6 x 6 => 36bits
        }
        else if (charcode >= 0xFC) {
            charcode &= 0x1; // 6
            lg = 5; // 5 x 6 + 1 => 31bits
        }
        else if (charcode >= 0xF8) {
            charcode &= 0x3; // 5
            lg = 4; // 4 x 6 + 2 => 26bits
        }
        else if (charcode >= 0xF0) {
            charcode &= 0x07; // 4
            lg = 3; // 3 x 6 + 3 => 21bits
        }
        else if (charcode >= 0xE0) {
            charcode &= 0x0F; // 3
            lg = 2; // 2 x 6 + 4 => 16bits
        }
        else if (charcode >= 0xC0) {
            charcode &= 0x1F; // 2
            lg = 1; // 6 + 5 => 11bits
        }

        while (lg-- > 0) {
            charcode = charcode << 6 | ((unsigned char*)str)[0] & 0x3f;
            str++;
        }
    }

    *txt = str;
    return charcode;
}

int ft_box(FT_Face face, const char* str)
{
    // memset(clip, 0, sizeof(*clip));
    int width = 0, yBearing = 0, yLower = 0;


    FT_GlyphSlot slot = face->glyph;
    while (*str) {
        int charcode = unichar(&str);

        FT_UInt glyph_index = FT_Get_Char_Index(face, charcode);
        fterror = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);

        yBearing = MAX(yBearing, slot->metrics.horiBearingY);
        yLower = MIN(yLower, slot->metrics.horiBearingY - slot->metrics.height);

        width += slot->advance.x >> 6;
    }

    yBearing = yBearing >> 6;
    yLower = yLower >> 6;
    return width;
}

void ft_write(gfx_t* screen, FT_Face face, float sizeInPts, const char *str, gfx_clip_t *clip, uint32_t color, int flags)
{
    int dpi = 96;
    int sizeInPx = (sizeInPts * dpi) / 96;
    fterror = FT_Set_Char_Size(face, 0, (int)(sizeInPts * 64.0f)/* 1/64th pts*/, dpi, dpi);

    int width = ft_box(face, str);
    color = color & 0xffffff;
    int x = clip->left + (clip->right - clip->left - width) / 2;
    int y = clip->top + sizeInPx + (clip->bottom - (clip->top + sizeInPx)) / 2;

    FT_GlyphSlot slot = face->glyph;

    ft_box(face, str);

    while (*str) {
        int charcode = unichar(&str);

        FT_UInt glyph_index = FT_Get_Char_Index(face, charcode);
        fterror = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        // face->glyph->metrics
        fterror = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

        // Draw
        for (int i = 0; i < slot->bitmap.rows; ++i) {
            int py = y - slot->bitmap_top + i;
            for (int j = 0; j < slot->bitmap.width; ++j) {
                int px = x + slot->bitmap_left + j;
                int a = slot->bitmap.buffer[i * slot->bitmap.pitch + j];
                if (a > 0)
                    screen->pixels4[py * screen->width + px] = gfx_upper_alpha_blend(screen->pixels4[py * screen->width + px], color | (a << 24));
            }
        }

        x += slot->advance.x >> 6;
        y += slot->advance.y >> 6; /* not useful for now */
    }


}

void new_window()
{
    window_t* win = malloc(sizeof(window_t));
    win->id = ++win_ID;
    win->pid = 0;
    win->shm = 0;
    win->x = 0;
    win->y = 0;
    win->w = 200;
    win->h = 200;
    win->fpos = 0;
    win->win = gfx_create_surface(200, 200);
    win->color = rand() ^ (rand() << 16);
    gfx_map(win->win);
    gfx_fill(win->win, win->color, GFX_NOBLEND, NULL);

    mtx_lock(&BFL);
    ll_append(&win_list, &win->node);
    active = win;
    mtx_unlock(&BFL);
    gfx_invalid(screen);
}

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

void window_fast_move(window_t *win, int key)
{
    int pos = -1;
    if (key == 75) {
        printf("Move to left\n");
        pos = fast_move[win->fpos][0];
    }
    else if (key == 77) {
        printf("Move to right\n");
        pos = fast_move[win->fpos][1];
    }
    else if (key == 72) {
        pos = fast_move[win->fpos][2];
    }
    else if (key == 80) {
        pos = fast_move[win->fpos][3];
    }

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

    gfx_map(win->win);
    gfx_fill(win->win, win->color, GFX_NOBLEND, NULL);
}

void window_position(window_t* win, gfx_clip_t* rect)
{
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

void paint(gfx_t* screen)
{
    // Background color / wallpaper
    if (wallpaper == NULL)
        gfx_fill(screen, 0xf3e4c6, GFX_NOBLEND, NULL);
    else
        gfx_blit(screen, wallpaper, GFX_NOBLEND, NULL, NULL);


    // Windows
    mtx_lock(&BFL);
    window_t* win;
    for ll_each(&win_list, win, window_t, node) {

        gfx_clip_t winClip;
        window_position(win, &winClip);
        gfx_map(win->win);
        gfx_blit(screen, win->win, GFX_NOBLEND, &winClip, NULL);
        set_shadow(screen, &winClip, 3, 140);
    }
    mtx_unlock(&BFL);

    // Task bar
    gfx_clip_t barClip;
    barClip.left = 0;
    barClip.right = screen->width;
    barClip.bottom = screen->height;
    barClip.top = screen->height - 50;
    gfx_fill(screen, 0x343434, GFX_NOBLEND, &barClip);

    // Print Clock !!?
    gfx_clip_t dtDay;
    dtDay.left = screen->width - 42;
    dtDay.right = screen->width - 10;
    dtDay.top = screen->height - 50;
    dtDay.bottom = screen->height;
    ft_write(screen, baseFont, 20, "31", &dtDay, 0xffffff, 0);

    gfx_clip_t dtMon;
    dtMon.left = screen->width - 74;
    dtMon.right = screen->width - 42;
    dtMon.top = screen->height - 40;
    dtMon.bottom = screen->height - 25;
    ft_write(screen, baseFont, 10, "AUG", &dtMon, 0xffffff, 0);

    gfx_clip_t dtYear;
    dtYear.left = screen->width - 74;
    dtYear.right = screen->width - 42;
    dtYear.top = screen->height - 25;
    dtYear.bottom = screen->height - 10;
    ft_write(screen, baseFont, 10, "2020", &dtYear, 0xffffff, 0);

    gfx_clip_t clkClip;
    clkClip.left = screen->width - 140;
    clkClip.right = screen->width - 100;
    clkClip.bottom = screen->height;
    clkClip.top = screen->height - 50;
    ft_write(screen, baseFont, 20, "12:45", &clkClip, 0xffffff, 0);


    clkClip.left = 0;
    clkClip.right = 60;
    clkClip.bottom = screen->height;
    clkClip.top = screen->height - 50;
    ft_write(screen, iconFont, 20, "\xef\x80\x82", &clkClip, 0xffffff, 0);


    // Mouse
    gfx_clip_t mseClip;
    gfx_t* cursor = cursors[cursorIdx];
    if (cursor == NULL)
        cursor = cursors[0];
    mseClip.left = seat.mouse_x - cursor->width / 2;
    mseClip.right = seat.mouse_x + cursor->width / 2;
    mseClip.bottom = seat.mouse_y + cursor->height / 2;
    mseClip.top = seat.mouse_y - cursor->height / 2;
    gfx_blit(screen, cursor, GFX_CLRBLEND, &mseClip, NULL);


}

#define BUFSIZE 512
int main()
{
    char buffer[BUFSIZE];
#ifndef main
    const char* RESX = "./resx";
#else
    const char* RESX = "/mnt/cdrom/usr/resx";
#endif

    // Font loading...
    fterror = FT_Init_FreeType(&library);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "fonts/arial.ttf");
    fterror = FT_New_Face(library, buffer, 0, &baseFont);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "fonts/fa5-solid-900.otf");
    fterror = FT_New_Face(library, buffer, 0, &iconFont);

    // Mouse cursor loading...
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/default.bmp");
    cursors[CRS_DEFAULT] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/pointer.bmp");
    cursors[CRS_POINTER] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/wait.bmp");
    cursors[CRS_WAIT] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/move.bmp");
    cursors[CRS_MOVE] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/text.bmp");
    cursors[CRS_TEXT] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/help.bmp");
    cursors[CRS_HELP] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/progress.bmp");
    cursors[CRS_PROGRESS] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/not-allowed.bmp");
    cursors[CRS_NOT_ALLOWED] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/n-resize.bmp");
    cursors[CRS_RESIZE_N] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/s-resize.bmp");
    cursors[CRS_RESIZE_S] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/e-resize.bmp");
    cursors[CRS_RESIZE_E] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/w-resize.bmp");
    cursors[CRS_RESIZE_W] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/ne-resize.bmp");
    cursors[CRS_RESIZE_NE] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/sw-resize.bmp");
    cursors[CRS_RESIZE_SW] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/nw-resize.bmp");
    cursors[CRS_RESIZE_NW] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/se-resize.bmp");
    cursors[CRS_RESIZE_SE] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/col-resize.bmp");
    cursors[CRS_RESIZE_COL] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/row-resize.bmp");
    cursors[CRS_RESIZE_ROW] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/all-scroll.bmp");
    cursors[CRS_ALL_SCROLL] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/crosshair.bmp");
    cursors[CRS_CROSSHAIR] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/drop.bmp");
    cursors[CRS_DROP] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/no-drop.bmp");
    cursors[CRS_NODROP] = gfx_load_image(buffer);

    // Application synchronization loading
    mtx_init(&BFL, mtx_plain);
    thrd_t srv_thrd;
    //thrd_create(&srv_thrd, &service, NULL);
    // ll_init(&win_list);

    // Display loading...
#ifndef main
    screen = gfx_create_window(NULL, _16x10(720), 720);
#else
    int fb0 = open("/fb0", O_RDWR);
    int kdb = open("/kdb", O_RDONLY);
    screen = gfx_opend(fb0, kdb);
#endif
    gfx_keyboard_load(&seat);


    // User settings loading...
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "wallpaper.png");
    gfx_t* loadWallpaper = gfx_load_image(buffer);
    if (loadWallpaper != NULL) {
        wallpaper = gfx_create_surface(screen->width, screen->height);
        gfx_map(wallpaper);
        gfx_blit_scale(wallpaper, loadWallpaper, GFX_NOBLEND, NULL, NULL);
        gfx_destroy(loadWallpaper);
    }


    gfx_msg_t msg;
    for (;;) {
        gfx_poll(screen, &msg);
        gfx_handle(screen, &msg, &seat);
        if (msg.message == GFX_EV_QUIT)
            break;
        else if (msg.message == GFX_EV_PAINT) {
            gfx_map(screen);
            paint(screen);
            gfx_flip(screen);
            continue;
        } else if (msg.message == GFX_EV_TIMER)
            continue;


        mtx_lock(&BFL);
        window_t* win = NULL;
        for ll_each_reverse(&win_list, win, window_t, node) {
            gfx_clip_t rect;
            window_position(win, &rect);
            if (rect.left >= seat.mouse_x)
                continue;
            if (rect.top >= seat.mouse_y)
                continue;
            if (rect.right < seat.mouse_x)
                continue;
            if (rect.bottom < seat.mouse_y)
                continue;
            break;
        }
        // int wid = win ? win->id : -1;
        // printf("Event %d for %d at <%d, %d>\n", msg.message, wid, seat.mouse_x, seat.mouse_y);
        mtx_unlock(&BFL);

        switch (msg.message) {

        case GFX_EV_MOUSEMOVE:
            gfx_invalid(screen); // Invalid previous and next cursor position !
            if (grab != NULL) {
                // TODO -- Mode MOVE / RESIZE (L, R, T, B, TL, TR, BL, BR)
                grab->x += seat.rel_x;
                grab->y += seat.rel_y;
                if (grab->x < 10 - grab->w)
                    grab->x = 10 - grab->w;
                if (grab->y < 10 - grab->h)
                    grab->y = 10 - grab->h;
                if (grab->x > screen->width - 10)
                    grab->x = screen->width - 10;
                if (grab->y > screen->height - 60)
                    grab->y = screen->height - 60;
            }

            // printf("Mouse move <%d, %d> <%d, %d>\n", seat.mouse_x, seat.mouse_y);
            break;

        case GFX_EV_BTNUP:
            if (msg.param1 == 1)
                grab = NULL;
            // printf("Mouse btn up (%d)\n", msg.param1);
            break;

        case GFX_EV_BTNDOWN:
            if (win != NULL && win != active) {
                mtx_lock(&BFL);
                ll_remove(&win_list, &win->node);
                active = win;
                ll_append(&win_list, &win->node);
                mtx_unlock(&BFL);
                gfx_invalid(screen);
            }
            if (msg.param1 == 1)
                grab = win;
            // printf("Mouse btn down (%d)\n", msg.param1);
            break;

        case GFX_EV_KEYDOWN:
            // 16 - Q
            if (msg.param1 == 30) // A
                new_window();
            else if (msg.param1 == 31)
                cursorIdx = (cursorIdx + 1) % CRS_MAX;
            else if (seat.kdb_status == KMOD_LCTRL && active != NULL) {
                window_fast_move(active, msg.param1);
            }

            printf("Key down (%d . %o)\n", msg.param1, seat.kdb_status);
            break;

        case GFX_EV_KEYUP:
            printf("Key up (%d . %o)\n", msg.param1, seat.kdb_status);
            break;
        case GFX_EV_MOUSEWHEEL:
            printf("Mouse wheel (%d)\n", msg.param1);
            break;
        }
    }

    gfx_destroy(screen);
    return 0;
}
