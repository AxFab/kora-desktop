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

void menu_paint(gfx_t *screen, menu_t *menu)
{
    gfx_fill(screen, menu->bg_color, menu->bg_color >= 0x1000000 ? GFX_UPPER_BLEND : GFX_NOBLEND, &menu->box);
    if (menu->shadow)
        gr_shadow(screen, &menu->box, BORDER_SHADOW_SIZE, 140);

    int i = 0;
    int x = 0;
    mitem_t *item;
    for ll_each(&menu->list, item, mitem_t, node) {

        gfx_clip_t box;
        box.left = menu->box.left + (menu->vertical ? 0 : x);
        box.top = menu->box.top + (menu->vertical ? x : 0);
        box.right = menu->vertical ? menu->box.right : box.left + menu->item_size;
        box.bottom = menu->vertical ? box.top + menu->item_size : menu->box.bottom;

        // Over / Active
        if (menu->idx_over == i && menu->idx_active == i)
            gfx_fill(screen, menu->bg3_color, menu->bg3_color >= 0x1000000 ? GFX_UPPER_BLEND : GFX_NOBLEND, &box);
        else if (menu->idx_active == i)
            gfx_fill(screen, menu->bg2_color, menu->bg2_color >= 0x1000000 ? GFX_UPPER_BLEND : GFX_NOBLEND, &box);
        else if (menu->idx_over == i)
            gfx_fill(screen, menu->bg1_color, menu->bg1_color >= 0x1000000 ? GFX_UPPER_BLEND : GFX_NOBLEND, &box);

        // Icon
        if (item->color != 0) {
            gfx_clip_t ico;
            ico.left = box.left + menu->pad;
            ico.top = box.top + menu->pad;
            ico.right = box.left + menu->item_size - menu->pad;
            ico.bottom = box.top + menu->item_size - menu->pad;
            gfx_fill(screen, item->color, item->color >= 0x1000000 ? GFX_UPPER_BLEND : GFX_NOBLEND, &ico);
        }

        // Text
        if (item->text != NULL) {
            gfx_clip_t txt;
            txt.left = box.left + menu->item_size;
            txt.top = box.top + menu->pad;
            txt.right = box.right - menu->pad;
            txt.bottom = box.top + menu->item_size - menu->pad;
            gr_write(screen, menu->font_face, menu->font_size, item->text, &txt, menu->tx_color, RCT_LEFT);
        }
        // Other

        x += menu->item_size + menu->gap;
        i++;
    }
}

bool menu_mouse(menu_t *menu, int x, int y, bool looking)
{
    if (!looking || x < menu->box.left || y < menu->box.top || x >= menu->box.right || y >= menu->box.bottom) {
        menu->idx_over = -1;
        return false;
    }
    // MARGIN !
    int ds = menu->vertical ? y - menu->box.top : x - menu->box.left;
    int sz = menu->item_size + menu->gap;
    int over = ds / sz;
    int ps = ds % sz;

    if (over >= menu->list.count_ || ps > menu->item_size)
        over = -1;

    if (menu->idx_over != over) {
        if (menu->idx_over >= 0)
            mgr_invalid_screen(0, 0, 0, 0);
        if (over >= 0)
            mgr_invalid_screen(0, 0, 0, 0);
        menu->idx_over = over;
    }
    return true;
}

void menu_button(menu_t *menu, int btn, bool press)
{
    if (menu->idx_over < 0)
        return;

    if (btn == 1 && press)
        menu->idx_grab = menu->idx_over;
    else if (btn == 1) {
        if (menu->idx_grab == menu->idx_over && menu->idx_grab >= 0) {
            bool activate = menu->click != NULL && menu->click(menu, menu->idx_over);
            if (activate) {
                if (menu->idx_over >= 0)
                    mgr_invalid_screen(0, 0, 0, 0);
                if (menu->idx_active >= 0)
                    mgr_invalid_screen(0, 0, 0, 0);
                menu->idx_active = menu->idx_over;
            }
        }
        menu->idx_grab = -1;
    }
}

void menu_config(menu_t *menu)
{
    menu->bg_color = 0x343434;
    menu->bg1_color = 0x4c4c4c; // Over
    menu->bg2_color = 0x585858; // Active
    menu->bg3_color = 0x6e6e6e; // Over AND Active
    menu->tx_color = 0xffffff;
    menu->shadow = false;
    menu->vertical = false;

    menu->item_size = 50;
    menu->idx_active = -1;
    menu->idx_over = -1;
    menu->idx_grab = -1;
    menu->pad = 3;
    menu->gap = 1;
    menu->font_face = FNT_DEFAULT;
    menu->font_size = 14;
    menu->list.first_ = NULL;
    menu->list.last_ = NULL;
    menu->list.count_ = 0;
}

