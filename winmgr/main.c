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
#include "wns.h"
#include <keycodes.h>
#include "winmgr.h"
#include <sys/mman.h>

struct winmgr _;

void screens_loading()
{
    memset(&_, 0, sizeof(_));
    mtx_init(&_.lock, mtx_plain);

    // Display loading...
#ifndef main
    int w = 780; //  480 / 1080
    gfx_context("win32");
    _.screen = gfx_create_window(_16x10(w), w);
#else
    _.screen = gfx_open_surface("/dev/fb0");
    // gfx_open_input("/dev/kbd");
#endif
    _.seat = _.screen->seat;
    // gfx_keyboard_load(&_.seat); TODO -- Change keyboard layout!?

    gfx_timer(20, 20);
}

int main(int argc, char **argv)
{
    thrd_t th_srv, th_resx;
    screens_loading();
    thrd_create(&th_resx, (thrd_start_t)resx_loading, NULL);
    thrd_create(&th_srv, (thrd_start_t)wns_service, NULL);
    event_loop();
    gfx_destroy(_.screen);
    return 0;
}
