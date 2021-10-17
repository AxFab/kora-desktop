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


bool on_click_start(menu_t *menu, int idx)
{
    mitem_t *prg = ll_index(&_.app_list, idx, mitem_t, node);

    window_create(NULL, 200, 200);
    return false;
}

bool on_click_task(menu_t *menu, int idx)
{
    if (idx == 0) {
        _.show_menu = !_.show_menu;
        mgr_invalid_screen(0, 0, 0, 0); // TODO -- Menu size
    } else {
        mitem_t *mapp = ll_index(&_.task_menu.list, idx, mitem_t, node);
        app_t *app = mapp->data;
        if (app != NULL)
            window_focus(app->win);
    }

    return false;
}


static void resx_cursor(const char *resx)
{
    char buffer[BUFSIZ];

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

    _.show_cursor = true;
    gfx_t *screen = _.screen;
    mgr_invalid_screen(0, 0, screen->width, screen->height);
}

static void resx_apps(const char *resx)
{
    char buffer[BUFSIZ];

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
            // scanf();
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

static void resx_fonts(const char *resx)
{
    char buffer[BUFSIZ];

    // Font loading...
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "fonts/arial.ttf");
    _.fontFaces[FNT_DEFAULT] = gfx_font("Arial", 10, GFXFT_REGULAR);
    _.fontFaces[FNT_DEFAULT + 1] = gfx_font("Arial", 14, GFXFT_REGULAR);
    _.fontFaces[FNT_DEFAULT + 2] = gfx_font("Arial", 20, GFXFT_REGULAR);
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "fonts/fa5-solid-900.otf");
    _.fontFaces[FNT_ICON] = gfx_font("Font Awesome 5 Free", 20, GFXFT_SOLID);

    _.show_taskbar = true;
    gfx_t *screen = _.screen;
    mgr_invalid_screen(0, 0, screen->width, screen->height);
}

static void resx_wallpaper(const char *resx)
{
    char buffer[BUFSIZ];
    snprintf(buffer, BUFSIZ, "%s/%s", resx, "wallpaper.png");
    gfx_t *loadWallpaper = gfx_load_image(buffer);
    if (loadWallpaper != NULL) {
        gfx_t *wlp = gfx_create_surface(_.screen->width, _.screen->height);
        gfx_map(wlp);
        gfx_blit_scale(wlp, loadWallpaper, GFX_NOBLEND, NULL, NULL);
        gfx_destroy(loadWallpaper);
        _.wallpaper = wlp;
    }

    gfx_t *screen = _.screen;
    mgr_invalid_screen(0, 0, screen->width, screen->height);
}

void resx_loading()
{
    const char *resx = RESX;
    resx_cursor(resx);
    resx_apps(resx);
    resx_fonts(resx);
    resx_wallpaper(resx);
}
