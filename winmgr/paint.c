/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#include <time.h>
#include <math.h>


static void gr_shadow_corner(gfx_t* screen, int size, int sx, int ex, int sy, int ey, int mx, int my, int alpha)
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

void gr_shadow(gfx_t* screen, gfx_clip_t* rect, int size, int alpha)
{
    int* grad = malloc((size + 1) * sizeof(int));
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

void gr_write(gfx_t* screen, int face, float sizeInPts, const char* str, gfx_clip_t* clip, uint32_t color, int flags)
{
    if (face == 0 && sizeInPts == 20)
        face = 2;
    else if (face == 0 && sizeInPts == 14)
        face = 1;
    gfx_font_t* font = _.fontFaces[face];
    gfx_text_metrics_t m;
    gfx_mesure_text(font, str, &m);
    int w = clip->right - clip->left;
    int h = clip->bottom - clip->top;
    gfx_write(screen, font, str, color, clip->left + (w - m.width) / 2, clip->top + (h + m.baseline ) / 2, clip);
}


const char* month[] = {
    "JAN", "FEV", "MAR", "APR",
    "MAY", "JUN", "JUL", "AUG",
    "SEP", "OCT", "NOV", "DEC"
};

void mgr_invalid_screen(int x, int y, int w, int h)
{
    // TODO -- Keep track of what to redraw !
    _.invalid = true;
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

void mgr_paint(gfx_t* screen)
{
    // Background color / wallpaper
    if (_.wallpaper == NULL)
        gfx_fill(screen, 0xf3e4c6, GFX_NOBLEND, NULL);
    else
        gfx_blit(screen, _.wallpaper, GFX_NOBLEND, NULL, NULL);

    // Windows
    mtx_lock(&_.lock);
    window_t* win;
    for ll_each(&_.win_list, win, window_t, node) {
        gfx_clip_t winClip;
        window_position(win, &winClip);
        gfx_map(win->gfx);
        gfx_blit(screen, win->gfx, GFX_NOBLEND, &winClip, NULL);
        gr_shadow(screen, &winClip, BORDER_SHADOW_SIZE, 140);
    }
    mtx_unlock(&_.lock);

    if (_.show_taskbar) {
        if (_.show_menu) {
            // Start Menu
            menu_paint(screen, &_.start_menu);
        }

        // Task bar
        menu_paint(screen, &_.task_menu);
        mgr_paint_clock(screen);

        gfx_clip_t clkClip;
        clkClip.left = 0;
        clkClip.right = 50;
        clkClip.bottom = screen->height;
        clkClip.top = screen->height - 50;
        gr_write(screen, FNT_ICON, 20, "\xef\x80\x82", &clkClip, 0xffffff, 0);
    }

    // Mouse
    if (_.show_cursor) {
        gfx_clip_t mseClip;
        gfx_t* cursor = _.cursors[_.cursorIdx];
        if (cursor == NULL)
            cursor = _.cursors[0];
        if (cursor != NULL) {
            mseClip.left = _.seat->mouse_x - cursor->width / 2;
            mseClip.right = _.seat->mouse_x + cursor->width / 2;
            mseClip.bottom = _.seat->mouse_y + cursor->height / 2;
            mseClip.top = _.seat->mouse_y - cursor->height / 2;
            gfx_blit(screen, cursor, GFX_CLRBLEND, &mseClip, NULL);
        }
    }
}

