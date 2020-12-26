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

#if defined __USE_FT

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library library; /* handle to library */
FT_Error fterror;
FT_Face fontFaces[16];

int ft_box(FT_Face face, const char *str)
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

void ft_write(gfx_t *screen, FT_Face face, float sizeInPts, const char *str, gfx_clip_t *clip, uint32_t color, int flags)
{
    int dpi = 96;
    int sizeInPx = (sizeInPts * dpi) / 96;
    fterror = FT_Set_Char_Size(face, 0, (int)(sizeInPts * 64.0f)/* 1/64th pts*/, dpi, dpi);

    int width = ft_box(face, str);
    color = color & 0xffffff;
    int x = clip->left + (clip->right - clip->left - width) / 2;
    int y = clip->top + sizeInPx + (clip->bottom - (clip->top + sizeInPx)) / 2;
    if (flags & RCT_LEFT)
        x = clip->left;

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

#endif

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
#if defined __USE_FT
    ft_write(screen, fontFaces[face], sizeInPts, str, clip, color, flags);
#else
#endif
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

void config_loading()
{
    char buffer[BUFSIZE];

#ifndef main
    const char *RESX = "./resx";
#else
    const char *RESX = "/mnt/cdrom/usr/resx";
#endif

#if defined __USE_FT
    // Font loading...
    fterror = FT_Init_FreeType(&library);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "fonts/arial.ttf");
    fterror = FT_New_Face(library, buffer, 0, &fontFaces[FNT_DEFAULT]);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "fonts/fa5-solid-900.otf");
    fterror = FT_New_Face(library, buffer, 0, &fontFaces[FNT_ICON]);
#endif

    // Mouse cursor loading...
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/default.bmp");
    _.cursors[CRS_DEFAULT] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/pointer.bmp");
    _.cursors[CRS_POINTER] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/wait.bmp");
    _.cursors[CRS_WAIT] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/move.bmp");
    _.cursors[CRS_MOVE] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/text.bmp");
    _.cursors[CRS_TEXT] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/help.bmp");
    _.cursors[CRS_HELP] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/progress.bmp");
    _.cursors[CRS_PROGRESS] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/not-allowed.bmp");
    _.cursors[CRS_NOT_ALLOWED] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/n-resize.bmp");
    _.cursors[CRS_RESIZE_N] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/s-resize.bmp");
    _.cursors[CRS_RESIZE_S] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/e-resize.bmp");
    _.cursors[CRS_RESIZE_E] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/w-resize.bmp");
    _.cursors[CRS_RESIZE_W] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/ne-resize.bmp");
    _.cursors[CRS_RESIZE_NE] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/sw-resize.bmp");
    _.cursors[CRS_RESIZE_SW] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/nw-resize.bmp");
    _.cursors[CRS_RESIZE_NW] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/se-resize.bmp");
    _.cursors[CRS_RESIZE_SE] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/col-resize.bmp");
    _.cursors[CRS_RESIZE_COL] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/row-resize.bmp");
    _.cursors[CRS_RESIZE_ROW] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/all-scroll.bmp");
    _.cursors[CRS_ALL_SCROLL] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/crosshair.bmp");
    _.cursors[CRS_CROSSHAIR] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/drop.bmp");
    _.cursors[CRS_DROP] = gfx_load_image(buffer);
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "cursors/no-drop.bmp");
    _.cursors[CRS_NODROP] = gfx_load_image(buffer);

    // Application synchronization loading
    mtx_init(&_.lock, mtx_plain);
    thrd_t srv_thrd;
    //thrd_create(&srv_thrd, &service, NULL);
    // ll_init(&win_list);

    // Display loading...
#ifndef main
    _.screen = gfx_create_window(NULL, _16x10(720), 720);
#else
    int fb0 = open("/fb0", O_RDWR);
    int kdb = open("/kdb", O_RDONLY);
    _.screen = gfx_opend(fb0, kdb);
#endif
    gfx_keyboard_load(&_.seat);


    // User settings loading...
    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "wallpaper.png");
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

    snprintf(buffer, BUFSIZ, "%s/%s", RESX, "apps.cfg");
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


}


