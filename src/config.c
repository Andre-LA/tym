/**
 * config.c
 *
 * Copyright (c) 2017 endaaman
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "config.h"

typedef void (*VteSetColorFunc)(VteTerminal*, const GdkRGBA*);

static const unsigned VTE_CJK_WIDTH_NARROW = 1;
static const unsigned VTE_CJK_WIDTH_WIDE = 2;

static const char* APP_NAME = "tym";
static const char* CONFIG_FILE_NAME = "config.lua";
static const char* WITHOUT_CONFIG_SYMBOL = "NONE";

static const int DEFAULT_WIDTH = 80;
static const int DEFAULT_HEIGHT = 22;
static const char* FALL_BACK_SHELL = "/bin/sh";
static const char* CURSOR_BLINK_MODE_SYSTEM = "system";
static const char* CURSOR_BLINK_MODE_ON = "on";
static const char* CURSOR_BLINK_MODE_OFF = "off";

static const char* CJK_WIDTH_NARROW = "narrow";
static const char* CJK_WIDTH_WIDE = "wide";

static GList* str_config_fields = NULL;
static GList* int_config_fields = NULL;

void init_config_fields() {
  const char* base_str_config_fields[] = {
    "title",
    "shell",
    "font",
    "cursor_blink_mode",
    "cjk_width",
    "color_bold",
    "color_foreground",
    "color_background",
    "color_cursor",
    "color_cursor_foreground",
    "color_highlight",
    "color_highlight_foreground",
  };

  const char* base_int_config_fields[] = {
    "width",
    "height",
  };

  // Initialize string fields
  for (unsigned i = 0; i < sizeof(base_str_config_fields) / sizeof(char*); i++) {
    str_config_fields = g_list_append(str_config_fields, g_strdup(base_str_config_fields[i]));
  }
  // Append `color_123` field
  char numbered_color_key[10];
  for (unsigned i = 0; i < 256; i++) {
    sprintf(numbered_color_key, "color_%d", i);
    str_config_fields = g_list_append(str_config_fields, g_strdup(numbered_color_key));
  }

  // Initialize integer fields
  for (unsigned i = 0; i < sizeof(base_int_config_fields) / sizeof(char*); i++) {
    int_config_fields = g_list_append(int_config_fields, g_strdup(base_int_config_fields[i]));
  }
}

void close_config_fields() {
  g_list_foreach(str_config_fields, (GFunc)g_free, NULL);
  g_list_foreach(int_config_fields, (GFunc)g_free, NULL);
  g_list_free(str_config_fields);
  g_list_free(int_config_fields);
}

char* get_default_shell() {
  const char* shell_env = g_getenv("SHELL");
  if (shell_env) {
    return g_strdup(shell_env);
  }
  char* user_shell = vte_get_user_shell();
  if (user_shell) {
    return user_shell;
  }
  return g_strdup(FALL_BACK_SHELL);
}

VteCursorBlinkMode match_cursor_blink_mode(const char* str) {
  if (0 == g_strcmp0(str, CURSOR_BLINK_MODE_ON)) {
    return VTE_CURSOR_BLINK_ON;
  }
  if (0 == g_strcmp0(str, CURSOR_BLINK_MODE_OFF)) {
    return VTE_CURSOR_BLINK_OFF;
  }
  return VTE_CURSOR_BLINK_SYSTEM;
}

unsigned match_cjk_width(const char* str) {
  if (0 == g_strcmp0(str, CJK_WIDTH_WIDE)) {
    return VTE_CJK_WIDTH_WIDE;
  }
  return VTE_CJK_WIDTH_NARROW;
}

char* config_get_str(Config* c, const char* key) {
  char* ptr = (char*)g_hash_table_lookup(c->context, key);
  if (!ptr) {
    g_print("warining: tried to refer null string field: `%s`.\n", key);
  }
  return ptr;
}

int config_get_int(Config* c, const char* key) {
  int* ptr = (int*)g_hash_table_lookup(c->context, key);
  if (!ptr) {
    g_print("warining: tried to refer null integer field: `%s`.\n", key);
    return 0;
  }
  return *ptr;
}

bool config_has(Config* c, const char* key) {
  char* value = config_get_str(c, key);
  if (!value) {
    return false;
  }
  if (0 == g_strcmp0(value, "")) {
    return false;
  }
  return true;
}

void config_set_str(Config* c, const char* key, const char* value) {
  char* old_key = NULL;
  bool has_value = g_hash_table_lookup_extended(c->context, key, (gpointer)&old_key, NULL);
  if (has_value) {
    g_hash_table_remove(c->context, old_key);
  }
  g_hash_table_insert(c->context, g_strdup(key), g_strdup(value));
}


void config_set_int(Config* c, const char* key, int value) {
  char* old_key = NULL;
  bool has_value = g_hash_table_lookup_extended(c->context, key, (gpointer)&old_key, NULL);
  if (has_value) {
    g_hash_table_remove(c->context, old_key);
  }
  g_hash_table_insert(c->context, g_strdup(key), g_memdup((gpointer)&value, sizeof(int)));
}

Config* config_init(const char* file_path) {
  Config* c = g_malloc0(sizeof(Config));
  c->context = g_hash_table_new_full(
    g_str_hash,
    g_str_equal,
    (GDestroyNotify)g_free,
    (GDestroyNotify)g_free
  );

  if (g_strcmp0(file_path, WITHOUT_CONFIG_SYMBOL) == 0) {
    // If symbol to start without config provived
    c->file_path = NULL;
    g_print("info: started with the default config\n");
  } else if (!file_path) {
    // If NULL
    c->file_path = g_build_path(
      G_DIR_SEPARATOR_S,
      g_get_user_config_dir(),
      APP_NAME,
      CONFIG_FILE_NAME,
      NULL
    );
  } else {
    c->file_path = g_strdup(file_path);
  }
  return c;
}

void config_close(Config* c) {
  g_hash_table_destroy(c->context);
  g_free(c->file_path);
  g_free(c);
}

void config_reset(Config* c) {
  char* default_shell = get_default_shell();
  config_set_str(c, "shell", default_shell);
  g_free(default_shell);
  config_set_str(c, "title", APP_NAME);
  config_set_str(c, "font", "");
  config_set_str(c, "cjk_width", CJK_WIDTH_NARROW);
  config_set_str(c, "cursor_blink_mode", CURSOR_BLINK_MODE_SYSTEM);
  for (GList* li = str_config_fields; li != NULL; li = li->next) {
    const char* key = (char *)li->data;
    // Set empty value if start with "color_"
    if (0 == g_ascii_strncasecmp(key, "color_", 6)) {
      config_set_str(c, key, "");
    }
  }
  config_set_int(c, "width", DEFAULT_WIDTH);
  config_set_int(c, "height", DEFAULT_HEIGHT);
}

void config_load(Config* c) {
  config_reset(c);

  // If file_path is NULL
  if (!c->file_path) {
    return;
  }

  // If config file does not exist
  if (!g_file_test(c->file_path, G_FILE_TEST_EXISTS)) {
    g_print("warining: `%s` does not exist\n", c->file_path);
    g_free(c->file_path);
    c->file_path = NULL;
    return;
  }

  lua_State* l = luaL_newstate();
  luaL_openlibs(l);

  lua_newtable(l);
  // Push string fields
  for (GList* li = str_config_fields; li != NULL; li = li->next) {
    const char* key = (char*)li->data;
    const char* value = config_get_str(c, key);
    lua_pushstring(l, key);
    lua_pushstring(l, value ? value : "");
    lua_settable(l, -3);
  }
  // Push int fields
  for (GList* li = int_config_fields; li != NULL; li = li->next) {
    const char* key = (char*)li->data;
    int value = config_get_int(c, key);
    lua_pushstring(l, key);
    lua_pushinteger(l, value);
    lua_settable(l, -3);
  }
  lua_setglobal(l, "config");

  // Run user config file
  luaL_loadfile(l, c->file_path);

  // If error
  if (lua_pcall(l, 0, 0, 0)) {
    g_print("warining: config error %s\n", lua_tostring(l, -1));
    g_print("warining: start with default configuration...\n");
    return;
  }

  lua_getglobal(l, "config");
  // Store string fields
  for (GList* li = str_config_fields; li != NULL; li = li->next) {
    const char* key = (char *)li->data;
    lua_getfield(l, -1, key);
    config_set_str(c, key, lua_tostring(l, -1));
    lua_pop(l, 1);
  }
  // Store int fields
  for (GList* li = int_config_fields; li != NULL; li = li->next) {
    const char* key = (char *)li->data;
    lua_getfield(l, -1, key);
    config_set_int(c, key, lua_tointeger(l, -1));
    lua_pop(l, 1);
  }
  lua_close(l);
}

void config_apply_color(
  Config* c,
  VteTerminal* vte,
  VteSetColorFunc vte_set_color_func,
  const char* key
) {
  if (!config_has(c, key)) {
    return;
  }
  GdkRGBA color;
  const char* value = config_get_str(c, key);
  bool valid = gdk_rgba_parse(&color, value);
  if (valid) {
    vte_set_color_func(vte, &color);
  }
}

void config_apply_colors(Config* c, VteTerminal* vte) {
  GdkRGBA* palette = g_new0(GdkRGBA, 256);
  char key[10];
  for (unsigned i = 0; i < 256; i++) {
    sprintf(key, "color_%d", i);
    if (config_has(c, key)) {
      bool valid = gdk_rgba_parse(&palette[i], config_get_str(c, key));
      if (valid) {
        continue;
      }
    }
    if (i < 16) {
      palette[i].blue = (((i & 4) ? 0xc000 : 0) + (i > 7 ? 0x3fff: 0)) / 65535.0;
      palette[i].green = (((i & 2) ? 0xc000 : 0) + (i > 7 ? 0x3fff : 0)) / 65535.0;
      palette[i].red = (((i & 1) ? 0xc000 : 0) + (i > 7 ? 0x3fff : 0)) / 65535.0;
      palette[i].alpha = 0;
      continue;
    }
    if (i < 232) {
      const unsigned j = i - 16;
      const unsigned r = j / 36, g = (j / 6) % 6, b = j % 6;
      const unsigned red =   (r == 0) ? 0 : r * 40 + 55;
      const unsigned green = (g == 0) ? 0 : g * 40 + 55;
      const unsigned blue =  (b == 0) ? 0 : b * 40 + 55;
      palette[i].red   = (red | red << 8) / 65535.0;
      palette[i].green = (green | green << 8) / 65535.0;
      palette[i].blue  = (blue | blue << 8) / 65535.0;
      palette[i].alpha = 0;
      continue;
    }
    const unsigned shade = 8 + (i - 232) * 10;
    palette[i].red = palette[i].green = palette[i].blue = (shade | shade << 8) / 65535.0;
    palette[i].alpha = 0;
  }
  vte_terminal_set_colors(vte, NULL, NULL, palette, 256);
}

void config_apply_all(Config* c, VteTerminal* vte, bool is_startup) {
  GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(vte));

  gtk_window_set_title(GTK_WINDOW(window), config_get_str(c, "title"));

  vte_terminal_set_cursor_blink_mode(vte, match_cursor_blink_mode(config_get_str(c, "cursor_blink_mode")));
  vte_terminal_set_cjk_ambiguous_width(vte, match_cjk_width(config_get_str(c, "cjk_width")));

  int width = config_get_int(c, "width"); // Number of horizontal char
  int height = config_get_int(c, "height"); // Number of vertical char
  if (is_startup){
    vte_terminal_set_size(vte, width, height);
  } else {
    GtkBorder border;
    gtk_style_context_get_padding(
      gtk_widget_get_style_context(GTK_WIDGET(vte)),
      gtk_widget_get_state_flags(GTK_WIDGET(vte)),
      &border
    );
    const int char_width = vte_terminal_get_char_width(vte);
    const int char_height = vte_terminal_get_char_height(vte);
    gtk_window_resize(
      GTK_WINDOW(window),
      width * char_width + border.left + border.right,
      height * char_height + border.top + border.bottom
    );
  }

  if (config_has(c, "font")) {
    PangoFontDescription* font_desc = pango_font_description_from_string(config_get_str(c, "font"));
    vte_terminal_set_font(vte, font_desc);
    pango_font_description_free(font_desc);
  }

  config_apply_colors(c, vte);
  config_apply_color(c, vte, vte_terminal_set_color_bold, "color_bold");
  config_apply_color(c, vte, vte_terminal_set_color_background, "color_background");
  config_apply_color(c, vte, vte_terminal_set_color_foreground, "color_foreground");
  config_apply_color(c, vte, vte_terminal_set_color_cursor, "color_cursor");
  config_apply_color(c, vte, vte_terminal_set_color_highlight, "color_highlight");
  config_apply_color(c, vte, vte_terminal_set_color_highlight_foreground, "color_highlight_foreground");
#if VTE_MAJOR_VERSION == 0
#if VTE_MINOR_VERSION >= 46
  /* vte_terminal_set_color_cursor_foreground is implemented since v0.46 */
  config_apply_color(c, vte, vte_terminal_set_color_cursor_foreground, "color_cursor_foreground");
#endif
#endif
}

__attribute__((constructor))
static void initialization() {
  init_config_fields();
}

__attribute__((destructor))
static void finalization() {
  close_config_fields();
}
