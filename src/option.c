/**
 * option.c
 *
 * Copyright (c) 2017 endaaman
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "option.h"
#include "config.h"


const char* OPTION_CONFIG_PREFIX = "c:";

// offset for "version" and "config_file_path"
const unsigned offset_data = 2;

Option* option_init() {
  Option* option = g_malloc0(sizeof(Option));

  unsigned size_entries = 0
    + ((sizeof(fields_int) + sizeof(fields_bool) + sizeof(fields_str)) / sizeof(char*))
    + offset_data
    + 1;
  GOptionEntry* entries = (GOptionEntry*)g_malloc0_n(sizeof(GOptionEntry), size_entries);

  option->config_str = (char**)g_malloc0_n(sizeof(char*), sizeof(fields_str) / sizeof(char*));
  option->config_int = (int*)g_malloc0_n(sizeof(int), sizeof(fields_int) / sizeof(char*));
  option->config_bool = (char**)g_malloc0_n(sizeof(char*), sizeof(fields_bool) / sizeof(char*));

  entries[0].long_name = g_strdup("version");
  entries[0].short_name = 'v';
  entries[0].flags = G_OPTION_FLAG_NONE;
  entries[0].arg = G_OPTION_ARG_NONE;
  entries[0].arg_data = &option->version;
  entries[0].description = "Show version";
  entries[0].arg_description = NULL;

  entries[1].long_name = g_strdup("use");
  entries[1].short_name = 'u';
  entries[1].flags = G_OPTION_FLAG_NONE;
  entries[1].arg = G_OPTION_ARG_STRING;
  entries[1].arg_data = &option->config_file_path;
  entries[1].description = "Use <path> instead of default config file";
  entries[1].arg_description = g_strdup("<path>");

  unsigned cur = offset_data;
  unsigned i;
  unsigned size;

  i = 0;
  size = sizeof(fields_str) / sizeof(char*);
  option->idx_config_str = cur;
  while (i < size) {
    const char* key = fields_str[i];
    entries[cur].long_name = g_strdup_printf("%s%s", OPTION_CONFIG_PREFIX, key);
    entries[cur].short_name = 0;
    entries[cur].flags = G_OPTION_FLAG_NONE;
    entries[cur].arg = G_OPTION_ARG_STRING;
    entries[cur].arg_data = &option->config_str[i];
    entries[cur].description = NULL;
    entries[cur].arg_description = "<string>";
    cur++;
    i++;
  }

  i = 0;
  size = sizeof(fields_int) / sizeof(char*);
  option->idx_config_int = cur;
  while (i < size) {
    const char* key = fields_int[i];
    entries[cur].long_name = g_strdup_printf("%s%s", OPTION_CONFIG_PREFIX, key);
    entries[cur].short_name = 0;
    entries[cur].flags = G_OPTION_FLAG_NONE;
    entries[cur].arg = G_OPTION_ARG_INT;
    entries[cur].arg_data = &option->config_int[i];
    entries[cur].description = NULL;
    entries[cur].arg_description = "<integer>";
    cur++;
    i++;
  }

  i = 0;
  size = sizeof(fields_bool) / sizeof(char*);
  option->idx_config_bool = cur;
  while (i < size) {
    const char* key = fields_bool[i];
    entries[cur].long_name = g_strdup_printf("%s%s", OPTION_CONFIG_PREFIX, key);
    entries[cur].short_name = 0;
    entries[cur].flags = G_OPTION_FLAG_NONE;
    entries[cur].arg = G_OPTION_ARG_STRING;
    entries[cur].arg_data = &option->config_bool[i];
    entries[cur].description = NULL;
    entries[cur].arg_description = "<true/false>";
    cur++;
    i++;
  }

  option->entries = entries;
  return option;
}

void option_close(Option* option) {
  unsigned i = 0;
  unsigned size = sizeof(option->entries) / sizeof(char*);
  while (i < size) {
    GOptionEntry* e = &option->entries[i];
    g_free((char*)e->long_name);
    if (e->arg_description) {
      g_free((char*)e->arg_description);
    }
    i++;
  }
  g_free(option->entries);
  g_free(option->config_str);
  g_free(option->config_bool);
  g_free(option->config_int);
  g_free(option);
}

bool option_check(Option* option, int* argc, char*** argv, GError** error) {
  GOptionContext* option_context = g_option_context_new(NULL);
  g_option_context_add_main_entries(option_context, option->entries, NULL);

  if (!g_option_context_parse(option_context, argc, argv, error)) {
    g_option_context_free(option_context);
    return false;
  }

  if (option->version) {
    g_option_context_free(option_context);
    g_print("version %s\n", PACKAGE_VERSION);
    return false;
  }

  g_option_context_free(option_context);
  return true;
}
