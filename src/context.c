/**
 * context.c
 *
 * Copyright (c) 2017 endaaman
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "context.h"
#include "command.h"
#include "builtin.h"


static const char* CONFIG_FILE_NAME = "config.lua";
static const char* USE_DEFAULT_CONFIG_SYMBOL = "NONE";
static const char* CONFIG_DIR_NAME = "tym";
static const char* LIB_NAME = "tym";

static void context_embed_builtin_functions(Context* context); // declare forward


Context* context_init(const char* config_file_path, GtkApplication* app, VteTerminal* vte)
{
  dd("context init");

  Context* context = g_malloc0(sizeof(Context));

  lua_State* l = luaL_newstate();
  luaL_openlibs(l);
  context->lua = l;

  if (0 == g_strcmp0(config_file_path, USE_DEFAULT_CONFIG_SYMBOL)) {
    // If symbol to start without config provived
    context->config_file_path = NULL;
    g_print("info: started with the default config\n");
  } else if (!config_file_path) {
    // If NULL
    context->use_default_config_file = true;
    context->config_file_path = g_build_path(
      G_DIR_SEPARATOR_S,
      g_get_user_config_dir(),
      CONFIG_DIR_NAME,
      CONFIG_FILE_NAME,
      NULL
    );
  } else {
    context->config_file_path = g_strdup(config_file_path);
  }

  context->app = app;
  context->vte = vte;

  context->config = config_init(l);
  context->keymap = keymap_init(l);

  context_embed_builtin_functions(context);

  return context;
}

void context_close(Context* context)
{
  dd("context close");

  lua_close(context->lua);
  g_free(context->config_file_path);
  config_close(context->config);
  keymap_close(context->keymap);
  g_free(context);
}

void context_load_config(Context* context, bool is_startup)
{
  dd("context load config");

  config_reset(context->config);
  keymap_reset(context->keymap);

  if (!context->config_file_path) {
    // Running without config
    return;
  }

  if (!g_file_test(context->config_file_path, G_FILE_TEST_EXISTS)) {
    // Warn only if user config file provided
    if (!context->use_default_config_file) {
      g_print("warning: `%s` does not exist\n", context->config_file_path);
    }
    return;
  }

  config_prepare_lua(context->config);
  keymap_prepare_lua(context->keymap);

  lua_State* l = context->lua;
  luaL_loadfile(l, context->config_file_path);

  if (lua_pcall(l, 0, 0, 0)) {
    const char* error = lua_tostring(l, -1);
    g_print("warning: config error %s\n", error);
    g_print("warning: start with default configuration...\n");
    command_notify(context, error, "tym: config load error");
    return;
  }

  config_load_from_lua(context->config);
  keymap_load_from_lua(context->keymap);

  config_apply_all(context->config, context->vte, is_startup);
}


static void context_embed_builtin_functions(Context* context)
{
  const luaL_Reg table[] = {
    { "get_version",          builtin_get_version },
    { "get_config_file_path", builtin_get_config_file_path },
    { "notify",               builtin_notify },
    { "put",                  builtin_put },
    { "reload",               builtin_reload },
    { "increase_font_scale",  builtin_increase_font_scale },
    { "decrease_font_scale",  builtin_decrease_font_scale },
    { "reset_font_scale",     builtin_reset_font_scale },
    { "copy_clipboard",       builtin_copy_clipboard },
    { "paste_clipboard",      builtin_paste_clipboard },
    { NULL, NULL },
  };

  lua_State* l = context->lua;
  luaL_newlibtable(l, table);
  lua_pushlightuserdata(l, context);
  luaL_setfuncs(l, table, 1);
  lua_setglobal(l, LIB_NAME);
}

bool context_on_key(Context* context, unsigned key, GdkModifierType mod)
{
  if (keymap_perform_custom(context->keymap, key, mod)) {
    return true;
  }
  if (config_get_use_default_keymap(context->config) && keymap_perform_default(context->keymap, context, key, mod)) {
    return true;
  }
  return false;
}
