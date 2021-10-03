#include "winmgr.h"
#include <math.h>

#define BUFSIZE 512

int unichar(const char **txt)
{
    const char *str = *txt;
    int charcode = ((unsigned char *)str)[0];
    str++;

    if (charcode & 0x80) { // mbstring !
        int lg = 1;
        if (charcode >= 0xFF) {
            charcode = 0; // 8
            lg = 0; // 7 x 6 => 42bits
        } else if (charcode >= 0xFE) {
            charcode = 0; // 7
            lg = 0; // 6 x 6 => 36bits
        } else if (charcode >= 0xFC) {
            charcode &= 0x1; // 6
            lg = 5; // 5 x 6 + 1 => 31bits
        } else if (charcode >= 0xF8) {
            charcode &= 0x3; // 5
            lg = 4; // 4 x 6 + 2 => 26bits
        } else if (charcode >= 0xF0) {
            charcode &= 0x07; // 4
            lg = 3; // 3 x 6 + 3 => 21bits
        } else if (charcode >= 0xE0) {
            charcode &= 0x0F; // 3
            lg = 2; // 2 x 6 + 4 => 16bits
        } else if (charcode >= 0xC0) {
            charcode &= 0x1F; // 2
            lg = 1; // 6 + 5 => 11bits
        }

        while (lg-- > 0) {
            charcode = charcode << 6 | ((unsigned char *)str)[0] & 0x3f;
            str++;
        }
    }

    *txt = str;
    return charcode;
}

gfx_font_t *fontFaces[16];


static void gr_shadow_corner(gfx_t *screen, int size, int sx, int ex, int sy, int ey, int mx, int my, int alpha)
{
    for (int i = sy; i < ey; ++i)
        for (int j = sx; j < ex; ++j) {
            int dm = abs(j - mx) + abs(i - my);
            float l = abs(j - mx) * abs(j - mx) + abs(i - my) * abs(i - my);
            float l2 = sqrt(l);
            float d = l2 / (float)(size);
            if (d < 1.0f) {
                int a = (1 - d) * (float)alpha;
                screen->pixels4[i * screen->width + j] = gfx_upper_alpha_blend(screen->pixels4[i * screen->width + j], a << 24);
            }
        }
}

void gr_shadow(gfx_t *screen, gfx_clip_t *rect, int size, int alpha)
{
    int *grad = malloc((size + 1) * sizeof(int));
    for (int i = 0; i < size + 1; ++i)
        grad[i] = (1.0 - sqrt(i / (float)size)) * (float)alpha;

    // TOP LEFT CORNER
    int miny = MAX(0, rect->top - size);
    int maxy = MAX(0, rect->top);
    int minx = MAX(0, rect->left - size);
    int maxx = MAX(0, rect->left);
    gr_shadow_corner(screen, size, minx, maxx, miny, maxy, maxx, maxy, alpha);


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


    gr_shadow_corner(screen, size, minx2, maxx2, miny, maxy, minx2, maxy, alpha);
    gr_shadow_corner(screen, size, minx, maxx, miny2, maxy2, maxx, miny2, alpha);
    gr_shadow_corner(screen, size, minx2, maxx2, miny2, maxy2, minx2, miny2, alpha);

    free(grad);
}

void gr_write(gfx_t *screen, int face, float sizeInPts, const char *str, gfx_clip_t *clip, uint32_t color, int flags)
{
    if (face == 0 && sizeInPts == 20)
        face = 2;
    else if (face == 0 && sizeInPts == 14)
        face = 1;
    gfx_font_t* font = fontFaces[face];
    gfx_text_metrics_t m;
    gfx_mesure_text(font, str, &m);
    int w = clip->right - clip->left;
    int h = clip->bottom - clip->top;
    gfx_write(screen, font, str, color, clip->left + (w - m.width) / 2, clip->top + (h - m.height) / 2 + m.baseline * 1.25, clip);
}


bool on_click_start(menu_t *menu, int idx)
{
    window_create();
    return false;
}

bool on_click_task(menu_t *menu, int idx)
{
    if (idx == 0) {
        _.show_menu = !_.show_menu;
        mgr_invalid_screen(0, 0, 0, 0);
    } else {
        app_t *app = ll_index(&_.app_list, idx - 1, app_t, node);
        if (app != NULL)
            window_focus(app->win);
    }

    return false;
}

void config_loading(const char *resx)
{
    char buffer[BUFSIZE];

    // Font loading...
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "fonts/arial.ttf");
    fontFaces[FNT_DEFAULT] = gfx_font("Arial", 10, 0);
    fontFaces[FNT_DEFAULT + 1] = gfx_font("Arial", 14, 0);
    fontFaces[FNT_DEFAULT + 2] = gfx_font("Arial", 20, 0);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "fonts/fa5-solid-900.otf");
    fontFaces[FNT_ICON] = gfx_font("Font Awesome", 20, 0);

    // Mouse cursor loading...
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/default.bmp");
    _.cursors[CRS_DEFAULT] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/pointer.bmp");
    _.cursors[CRS_POINTER] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/wait.bmp");
    _.cursors[CRS_WAIT] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/move.bmp");
    _.cursors[CRS_MOVE] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/text.bmp");
    _.cursors[CRS_TEXT] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/help.bmp");
    _.cursors[CRS_HELP] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/progress.bmp");
    _.cursors[CRS_PROGRESS] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/not-allowed.bmp");
    _.cursors[CRS_NOT_ALLOWED] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/n-resize.bmp");
    _.cursors[CRS_RESIZE_N] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/s-resize.bmp");
    _.cursors[CRS_RESIZE_S] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/e-resize.bmp");
    _.cursors[CRS_RESIZE_E] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/w-resize.bmp");
    _.cursors[CRS_RESIZE_W] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/ne-resize.bmp");
    _.cursors[CRS_RESIZE_NE] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/sw-resize.bmp");
    _.cursors[CRS_RESIZE_SW] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/nw-resize.bmp");
    _.cursors[CRS_RESIZE_NW] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/se-resize.bmp");
    _.cursors[CRS_RESIZE_SE] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/col-resize.bmp");
    _.cursors[CRS_RESIZE_COL] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/row-resize.bmp");
    _.cursors[CRS_RESIZE_ROW] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/all-scroll.bmp");
    _.cursors[CRS_ALL_SCROLL] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/crosshair.bmp");
    _.cursors[CRS_CROSSHAIR] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/drop.bmp");
    _.cursors[CRS_DROP] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "cursors/no-drop.bmp");
    _.cursors[CRS_NODROP] = gfx_load_image(buffer);

    // Application synchronization loading
    mtx_init(&_.lock, mtx_plain);
    thrd_t srv_thrd;
    //thrd_create(&srv_thrd, &service, NULL);
    // ll_init(&win_list);

    // Display loading...
#ifndef main
    // int w = 480;
    int w = 780;
    // int w = 1080;
    _.screen = gfx_create_window(NULL, _16x10(w), w, 0);
#else
    _.screen = gfx_open_surface("/dev/fb0");
    // gfx_open_input("/dev/kbd");
#endif
    _.seat = _.screen->seat;
    // gfx_keyboard_load(&_.seat); TODO -- Change keyboard layout!?

    // User settings loading...
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "wallpaper.png");
    gfx_t *loadWallpaper = gfx_load_image(buffer);
    if (loadWallpaper != NULL) {
        _.wallpaper = gfx_create_surface(_.screen->width, _.screen->height);
        gfx_map(_.wallpaper);
        gfx_blit_scale(_.wallpaper, loadWallpaper, GFX_NOBLEND, NULL, NULL);
        gfx_destroy(loadWallpaper);
    }


    // Build start menu
    menu_config(&_.start_menu);
    _.start_menu.box.left = 0;
    _.start_menu.box.top = _.screen->height / 3;
    _.start_menu.box.right = 240;
    _.start_menu.box.bottom = _.screen->height - 50;
    _.start_menu.bg_color = 0xE0343434;
    _.start_menu.shadow = true;
    _.start_menu.vertical = true;
    _.start_menu.item_size = 42;
    _.start_menu.pad = 5;
    _.start_menu.gap = 2;
    _.start_menu.click = on_click_start;

    mitem_t *item;

    snprintf(buffer, BUFSIZ, "%s/%s", resx, "apps.cfg");
    FILE *fp = fopen(buffer, "r");
    if (fp != NULL) {
        char buf[512];
        while (fgets(buf, 512, fp)) {

            item = malloc(sizeof(mitem_t));
            item->color = 0xa61010;
            item->text = strdup(strtok(buf, ";\n")); // "Krish";
            // Command: strdup(strtok(NULL, ";\n"));
            // Icon: strdup(strtok(NULL, ";\n"));
            ll_append(&_.start_menu.list, &item->node);
        }
    }

    // Task Menu
    menu_config(&_.task_menu);
    _.task_menu.box.left = 0;
    _.task_menu.box.top = _.screen->height - 50;
    _.task_menu.box.right = _.screen->width;
    _.task_menu.box.bottom = _.screen->height;
    _.task_menu.item_size = 50;
    _.task_menu.pad = 4;
    _.task_menu.gap = 2;
    _.task_menu.click = on_click_task;

    item = malloc(sizeof(mitem_t));
    item->color = 0;
    item->text = NULL;
    ll_append(&_.task_menu.list, &item->node);

    gfx_timer(20, 20);
}


