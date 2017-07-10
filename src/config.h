/**
 * config.h
 *
 * Copyright (c) 2017 endaaman
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <stdbool.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

GHashTable* config_init();
void config_close(GHashTable* config);

char* config_get_str(GHashTable* c, const char* key);

void config_load(GHashTable* c);
void config_apply_all(GHashTable* c, VteTerminal* vte, bool is_startup);

#endif
