/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015  <Fabien Bavent>
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
#include <gum/core.h>
#include <gum/cells.h>
#include <gum/events.h>
#include <gum/xml.h>
#include <gfx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

GUM_cell *gum_debug_root(GUM_event_manager *evm);
void gum_debug_show_tree(GUM_cell *cell, int depth);


GUM_event_manager *evm;
GUM_cell *root;
GUM_skins *skins;

GUM_cell *user;
GUM_cell *users;
GUM_cell *ctx_lang;
GUM_cell *lbl_clock;

GUM_cell *selected_user;
char *selected_pwd;

void load_users();

void on_select(GUM_event_manager *evm, GUM_cell *cell, int event)
{
    GUM_cell *usr = users->first;
    while (usr) {
        gum_get_by_id(usr, "hid")->state |= GUM_CELL_HIDDEN;
        gum_invalid_measure(gum_get_by_id(usr, "hid"));
        gum_cell_set_text(gum_get_by_id(cell, "pwd"), "");
        gum_invalid_visual(usr);
        usr = usr->next;
    }

    if (cell != selected_user) {
        selected_user = cell;
        printf("Select user '%s' \n", gum_get_by_id(cell, "usr")->text);
        gum_get_by_id(cell, "hid")->state &= ~GUM_CELL_HIDDEN;
        gum_invalid_measure(gum_get_by_id(cell, "hid"));
        gum_set_focus(evm, gum_get_by_id(cell, "pwd"));
    } else if (selected_user != NULL) {
        printf("Unselect user\n");
        // gum_get_by_id(selected_user, "hid")->state |= GUM_CELL_HIDDEN;
        // gum_invalid_measure(gum_get_by_id(selected_user, "hid"));
        selected_user = NULL;
        gum_set_focus(evm, NULL);
    }

    // give focus
    gum_refresh(evm);
}

void *start_auth(GUM_event_manager *evm, void *arg)
{
    if (selected_user == NULL) {
        free(selected_pwd);
        return NULL;
    }
    // GUM_cell *usr = gum_get_by_id(selected_user, "usr");
    // GUM_cell *pwd = gum_get_by_id(selected_user, "pwd");

    // time_t start = time(NULL);
    void *desktop = calloc(48, 1);
    // LOAD USER AUTH INFO
    // CHECK PASSWORD
    // WAIT until elapsed < MIN_DELAY
    free(selected_pwd);
    if (!desktop)
        return NULL;
    // LOAD DESKTOP INFO
    // START DESKTOP PROCESS
    return desktop;
}

void on_auth_completed(GUM_event_manager *evm, void *arg) // ASYNC_CALLBACK
{
    GUM_cell *usr = gum_get_by_id(selected_user, "usr");
    GUM_cell *pwd = gum_get_by_id(selected_user, "pwd");
    if (arg == NULL) {
        printf("Login for '%s' wrong password\n", usr->text);
        gum_set_focus(evm, pwd);
        // TODO -- Buzz selected_user (animation)
        return;
    }

    // TRANSFERT DEVICES OWNERSHIP TO DESKTOP PROCESS
    printf("Login for '%s' sucessfull\n", usr->text);
    gum_cell_destroy_children(evm, users);
    // HIDE BUTTON PANEL...
    // START ASYNC TO OPEN DESKTOP....
}

void on_grab_ownership(GUM_event_manager *evm, GUM_cell *cell, int event)
{
    load_users();
}

void on_login(GUM_event_manager *evm, GUM_cell *cell, int event)
{
    if (selected_user == NULL)
        return;
    GUM_cell *usr = gum_get_by_id(selected_user, "usr");
    GUM_cell *pwd = gum_get_by_id(selected_user, "pwd");
    selected_pwd = strdup(pwd->text);
    printf("Login for user '%s' and password '%s' \n", usr->text, pwd->text);
    gum_cell_set_text(pwd, "");

    // Remove password field and display spinner
    // Block keyboard and button -- keep over?
    gum_async_worker(evm, start_auth, on_auth_completed, NULL);
}

void load_users()
{
    gum_cell_destroy_children(evm, users);
    FILE *info = fopen("./resx/password.txt", "r");
    if (info == NULL)
        return;
    char buf[512];
    while (fgets(buf, 512, info)) {
        GUM_cell *usr = gum_cell_copy(user);
        gum_get_by_id(usr, "usr")->text = strdup(strtok(buf, ";\n"));
        gum_get_by_id(usr, "img")->img_src = strdup(strtok(NULL, ";\n"));
        gum_event_bind(evm, usr, GUM_EV_CLICK, (void *)on_select, NULL);
        gum_event_bind(evm, gum_get_by_id(usr, "go"), GUM_EV_CLICK, (void *)on_login, NULL);
        gum_cell_pushback(users, usr);
    }
    gum_refresh(evm);
}


void on_lang(GUM_event_manager *evm, GUM_cell *cell, int event)
{
    printf("Language menu\n");
    gum_show_context(evm, ctx_lang);
}

#define SEC_PER_DAY  86400
#define SEC_PER_HOUR  3600
#define SEC_PER_MIN  60

#define MIN_PER_DAY  1440
#define MIN_PER_HOUR  60

#define HOUR_PER_DAY  24

int last = 0;

void on_tick(GUM_event_manager *evm, GUM_cell *cell, int event)
{
    char buf[12];
    int tz_off = 2 * MIN_PER_HOUR;
    int now = (time(NULL) / SEC_PER_MIN + tz_off) % MIN_PER_DAY;
    if (now == last)
        return;
    last = now;
    int hour = now / MIN_PER_HOUR;
    int min = now % MIN_PER_HOUR;
    snprintf(buf, 12, "%d:%02d", hour, min);
    // snprintf(buf, 12, "%d:%02d %s", hour % 12, min, hour >= 12 ? "PM" : "AM");
    gum_cell_set_text(lbl_clock, buf);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

GUM_window *gum_open_surface(gfx_t *gfx);

/* Graphical User-interface Module */
int main(int argc, char **argv, char **env)
{
    int width = 680 * 2;
    int height = width * 10 / 16; // 425

    // Load models
    skins = gum_skins_loadcss(NULL, "./resx/logon.css");
    root = gum_cell_loadxml("./resx/logon.xml", skins);
    if (root == NULL) {
        printf("Unable to create render model.\n");
        return EXIT_FAILURE;
    }

    // Open Window
    GUM_window *win = gum_create_surface(width, height);
    if (win == NULL) {
        printf("Unable to initialize window.\n");
        return EXIT_FAILURE;
    }

    users = gum_get_by_id(root, "users");
    user = gum_get_by_id(root, "user");
    gum_cell_detach(user);
    ctx_lang = gum_get_by_id(root, "ctx-lang");
    gum_cell_detach(ctx_lang);
    lbl_clock = gum_get_by_id(root, "btn-clock");

    evm = gum_event_manager(root, win);
    gum_event_bind(evm, gum_get_by_id(root, "btn-lang"), GUM_EV_CLICK, (void *)on_lang, NULL);
    gum_event_bind(evm, NULL, GUM_EV_TICK, (void *)on_tick, NULL);
    load_users();

    gum_event_loop(evm);

    gum_close_manager(evm);
    gum_destroy_surface(win);
    gum_destroy_cells(NULL, root);
    gum_destroy_cells(NULL, user);
    gum_destroy_cells(NULL, ctx_lang);
    gum_destroy_skins(skins);
    return EXIT_SUCCESS;
}


