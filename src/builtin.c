/**
 * builtin.c
 *
 * Copyright (c) 2017 endaaman
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "builtin.h"
#include "context.h"
#include "command.h"


static int builtin_get(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  const char* key = luaL_checkstring(L, 1);
  MetaEntry* e = meta_get_entry(context->meta, key);
  if (!e) {
    luaX_warn(L, "Invalid config key: '%s'", key);
    lua_pushnil(L);
    return 1;
  }

  switch (e->type) {
    case META_ENTRY_TYPE_STRING:
      lua_pushstring(L, config_get_str(context->config, key));
      break;
    case META_ENTRY_TYPE_INTEGER:
      lua_pushinteger(L, config_get_int(context->config, key));
      break;
    case META_ENTRY_TYPE_BOOLEAN:
      lua_pushboolean(L, config_get_bool(context->config, key));
      break;
    default:
      lua_pushnil(L);
      break;
  }
  return 1;
}

static int builtin_set(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  const char* key = luaL_checkstring(L, 1);

  MetaEntry* e = meta_get_entry(context->meta, key);
  if (!e) {
    luaX_warn(L, "Invalid config key: '%s'", key);
    return 0;
  }

  int type = lua_type(L, 2);
  switch (e->type) {
    case META_ENTRY_TYPE_STRING: {
      const char* value = lua_tostring(L, 2);
      if (!value) {
        luaX_warn(L, "Invalid string config for '%s' (string expected, got %s)", key, lua_typename(L, type));
        break;
      }
      config_set_str(context->config, key, value);
      break;
    }
    case META_ENTRY_TYPE_INTEGER: {
      if (type != LUA_TNUMBER) {
        luaX_warn(L, "Invalid integer config for '%s': %s (number expected, got %s)", key, lua_tostring(L, 2), lua_typename(L, type));
        break;
      }
      int value = lua_tointeger(L, 2);
      config_set_int(context->config, key, value);
      break;
    }
    case META_ENTRY_TYPE_BOOLEAN: {
      int value = lua_toboolean(L, 2);
      config_set_bool(context->config, key, value);
      break;
    }
    default:
      break;
  }
  return 0;
}

static int builtin_get_config(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  lua_newtable(L);

  MetaIter iter = {};
  char* key = NULL;
  MetaEntry* e = NULL;
  meta_iter_init(&iter, context->meta);
  while (meta_iter_next(&iter, &key, &e)) {
    char* key = e->name;
    lua_pushstring(L, key);
    switch (e->type) {
      case META_ENTRY_TYPE_STRING: {
        const char* value = config_get_str(context->config, key);
        lua_pushstring(L, value);
        break;
      }
      case META_ENTRY_TYPE_INTEGER:
        lua_pushinteger(L, config_get_int(context->config, key));
        break;
      case META_ENTRY_TYPE_BOOLEAN:
        lua_pushboolean(L, config_get_bool(context->config, key));
        break;
      case META_ENTRY_TYPE_NONE:
        lua_pop(L, 1);
        continue;
    }
    lua_settable(L, -3);
  }
  return 1;
}

static int builtin_set_config(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  luaL_argcheck(L, lua_istable(L, 1), 1, "table expected");

  lua_pushnil(L);
  while (lua_next(L, -2)) {
    lua_pushvalue(L, -2);
    const char* key = lua_tostring(L, -1);
    MetaEntry* e = meta_get_entry(context->meta, key);
    if (e) {
      int type = lua_type(L, -2);
      switch (e->type) {
        case META_ENTRY_TYPE_STRING: {
          const char* value = lua_tostring(L, -2);
          if (!value) {
            luaX_warn(L, "Invalid string config for '%s' (string expected, got %s)", key, lua_typename(L, type));
            break;
          }
          config_set_str(context->config, key, value);
          break;
        }
        case META_ENTRY_TYPE_INTEGER: {
          if (type != LUA_TNUMBER) {
            luaX_warn(L, "Invalid integer config for '%s': %s (number expected, got %s)", key, lua_tostring(L, -2), lua_typename(L, type));
            break;
          }
          int value = lua_tointeger(L, -2);
          config_set_int(context->config, key, value);
          break;
        }
        case META_ENTRY_TYPE_BOOLEAN: {
          int value = lua_toboolean(L, -2);
          config_set_bool(context->config, key, value);
          break;
        }
        default:
          break;
      }
    } else {
      luaX_warn(L, "Invalid config key: '%s'", key);
    }
    lua_pop(L, 2);
  }

  return 0;
}

static int builtin_reset_config(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  config_reset(context->config);
  return 0;
}

static int builtin_set_keymap(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  const char* key = luaL_checkstring(L, 1);
  luaL_argcheck(L, lua_isfunction(L, 2), 2, "function expected");

  int ref = luaL_ref(L, LUA_REGISTRYINDEX);
  bool ok = keymap_add_entry(context->keymap, key, ref);
  if (!ok) {
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
    luaX_warn(L, "Invalid accelerator: '%s'", key);
    return 0;
  }
  return 0;
}

static int builtin_unset_keymap(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  const char* key = luaL_checkstring(L, 1);
  bool removed = keymap_remove_entry(context->keymap, key);
  if (!removed) {
    luaX_warn(L, "Tried to remove en empty keymap '(%s') which is not assigned function to", key);
  }
  return 0;
}

static int builtin_set_keymaps(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  luaL_argcheck(L, lua_istable(L, 1), 1, "table expected");

  lua_pushnil(L);
  while (lua_next(L, -2)) {
    lua_pushvalue(L, -2);
    const char* key = lua_tostring(L, -1);
    if (!lua_isfunction(L, -2)) {
      luaX_warn(L, "Invalid value for '%s': function expected, got %s", key, lua_typename(L, lua_type(L, -2)));
    } else {
      lua_pushvalue(L, -2); // push function to stack top
      int ref = luaL_ref(L, LUA_REGISTRYINDEX);
      bool ok = keymap_add_entry(context->keymap, key, ref);
      if (!ok) {
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
        luaX_warn(L, "Invalid accelerator: '%s'", key);
      }
    }
    lua_pop(L, 2);
  }

  return 0;
}

static int builtin_reset_keymaps(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  keymap_reset(context->keymap);
  return 0;
}

static int builtin_set_hook(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  const char* key = luaL_checkstring(L, 1);
  luaL_argcheck(L, lua_isfunction(L, 2), 2, "function expected");
  int ref = luaL_ref(L, LUA_REGISTRYINDEX);
  int old_ref = -1;
  if (hook_set_ref(context->hook, key, ref, &old_ref)) {
    if (old_ref > 0) {
      dd("unref old ref");
      luaL_unref(L, LUA_REGISTRYINDEX, old_ref);
    }
    return 0;
  }
  luaL_unref(L, LUA_REGISTRYINDEX, ref);
  luaX_warn(L, "Invalid hook key: '%s'", key);
  return 0;
}

static int builtin_set_hooks(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  luaL_argcheck(L, lua_istable(L, 1), 1, "table expected");
  lua_pushnil(L);
  while (lua_next(L, -2)) {
    lua_pushvalue(L, -2);
    const char* key = lua_tostring(L, -1);
    if (!lua_isfunction(L, -2)) {
      luaX_warn(L, "Invalid value for '%s': function expected, got %s", key, lua_typename(L, lua_type(L, -2)));
    } else {
      lua_pushvalue(L, -2); // push function to stack top
      int ref = luaL_ref(L, LUA_REGISTRYINDEX);
      int old_ref = -1;
      int ok = hook_set_ref(context->hook, key, ref, &old_ref);
      if (old_ref > 0) {
        dd("unref old ref");
        luaL_unref(L, LUA_REGISTRYINDEX, old_ref);
      }
      if (!ok) {
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
        luaX_warn(L, "Invalid hook key: '%s'", key);
      }
    }
    lua_pop(L, 2);
  }
  return 0;
}

static int builtin_reload(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  context_load_config(context);
  context_load_theme(context);
  context_apply_config(context);
  context_apply_theme(context);
  return 0;
}

static int builtin_reload_theme(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  context_load_theme(context);
  context_apply_theme(context);
  return 0;
}

static int builtin_apply(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  context_apply_config(context);
  context_apply_theme(context);
  return 0;
}

static int builtin_send_key(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  UNUSED(context);
  const char* accelerator = luaL_checkstring(L, 1);
  unsigned key;
  GdkModifierType mod;
  gtk_accelerator_parse(accelerator, &key, &mod);
  if (0 == key && 0 == mod) {
    luaL_error(L, "Invalid accelerator: '%s'", accelerator);
    return 0;
  }
  GdkEvent* event = gdk_event_new(GDK_KEY_PRESS);
  if (context->device == NULL) {
    g_warning("Could not get input device.");
    return 0;
  }
  gdk_event_set_device(event, context->device);
  event->key.window = g_object_ref(gtk_widget_get_window(GTK_WIDGET(context_get_window(context))));
  event->key.send_event = false;
  event->key.time = GDK_CURRENT_TIME;
  event->key.state = mod;
  event->key.keyval = key;
  gtk_main_do_event((GdkEvent*)event);
  event->type = GDK_KEY_RELEASE;
  gtk_main_do_event((GdkEvent*)event);
  gdk_event_free((GdkEvent*)event);
  return 0;
}

typedef struct {
  Context* context;
  int ref;
} TimeoutNotation;

static int timeout_callback(void* user_data)
{
  TimeoutNotation* notation = (TimeoutNotation*) user_data;

  lua_State* L = notation->context->lua;
  lua_rawgeti(L, LUA_REGISTRYINDEX, notation->ref);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 1); // pop none-function
    dd("tried to call non-function");
    return false;
  }

  if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
    luaX_warn(L, "Error in timeout function: '%s'", lua_tostring(L, -1));
    lua_pop(L, 1); // error
    return false;
  }
  bool result = lua_toboolean(L, -1);
  lua_pop(L, 1);
  return result;
}

static int builtin_set_timeout(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  luaL_argcheck(L, lua_isfunction(L, 1), 1, "function expected");
  int interval = lua_tointeger(L, 2); // if non-number, falling back to 0

  lua_pushvalue(L, 1);
  int ref = luaL_ref(L, LUA_REGISTRYINDEX);
  TimeoutNotation* notation = g_malloc0(sizeof(TimeoutNotation));
  notation->context = context;
  notation->ref = ref;
  int tag = g_timeout_add_full(G_PRIORITY_DEFAULT, interval, (GSourceFunc)timeout_callback, notation, g_free);
  lua_pushinteger(L, tag);
  return 1;
}

static int builtin_clear_timeout(lua_State* L)
{
  int tag = luaL_checkinteger(L, 1);
  g_source_remove(tag);
  return 0;
}

static int builtin_put(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  const char* text = luaL_checkstring(L, -1);
  vte_terminal_feed_child(context_get_vte(context), text, -1);
  return 0;
}

static int builtin_bell(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  gdk_window_beep(context_get_gdk_window(context));
  return 0;
}

static int builtin_open(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  const char* uri = luaL_checkstring(L, -1);
  context_launch_uri(context, uri);
  return 0;
}

static int builtin_notify(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  const char* body = luaL_checkstring(L, 1);
  const char* title = lua_tostring(L, 2);
  context_notify(context, body, title);
  return 0;
}

static int builtin_copy(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  command_copy_clipboard(context);
  return 0;
}

static int builtin_paste(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  command_paste_clipboard(context);
  return 0;
}

static int builtin_increase_font_scale(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  command_increase_font_scale(context);
  return 0;
}

static int builtin_decrease_font_scale(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  command_decrease_font_scale(context);
  return 0;
}

static int builtin_reset_font_scale(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  command_reset_font_scale(context);
  return 0;
}

static int builtin_color_to_rgba(lua_State* L)
{
  GdkRGBA color;
  const char* str = luaL_checkstring(L, -1);
  bool valid = gdk_rgba_parse(&color, str);
  if (valid) {
    lua_pushinteger(L, (int)(color.red * 255 + 0.5));
    lua_pushinteger(L, (int)(color.green * 255 + 0.5));
    lua_pushinteger(L, (int)(color.blue * 255 + 0.5));
    lua_pushnumber(L, color.alpha);
  } else {
    luaX_warn(L, "Invalid color string: '%s'", str);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 4;
}

static int builtin_rgba_to_color(lua_State* L)
{
  int red = luaL_checknumber(L, 1);
  int green = luaL_checknumber(L, 2);
  int blue = luaL_checknumber(L, 3);
  double alpha = lua_isnone(L, 4) ? 1.0 : lua_tonumber(L, 4);
  char* str = g_strdup_printf("rgba(%d, %d, %d, %f)", red, green, blue, alpha);
  lua_pushstring(L, str);
  g_free(str);
  return 1;
}

static int builtin_rgb_to_hex(lua_State* L)
{
  int red = luaL_checknumber(L, 1);
  int green = luaL_checknumber(L, 2);
  int blue = luaL_checknumber(L, 3);
  char* hex = g_strdup_printf("#%x%x%x", red, green, blue);
  lua_pushstring(L, hex);
  g_free(hex);
  return 1;
}

static int builtin_check_mod(lua_State* L)
{
  const char* accelerator = luaL_checkstring(L, 1);
  unsigned key;
  GdkModifierType mod;
  gtk_accelerator_parse(accelerator, &key, &mod);
  GdkDisplay* display = gdk_display_get_default();
  GdkKeymap* kmap = gdk_keymap_get_for_display(display);
  unsigned current_mod = gdk_keymap_get_modifier_state(kmap);
  lua_pushboolean(L, mod & current_mod);
  return 1;
}

static int builtin_get_monitor_model(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  GdkDisplay* display = gdk_display_get_default();
  GdkMonitor* monitor = gdk_display_get_monitor_at_window(display, context_get_gdk_window(context));
  lua_pushstring(L, gdk_monitor_get_model(monitor));
  return 1;
}

static int builtin_get_version(lua_State* L)
{
  lua_pushstring(L, PACKAGE_VERSION);
  return 1;
}

static int builtin_get_config_path(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  char* path;
  context_acquire_config_path(context, &path);
  lua_pushstring(L, path);
  g_free(path);
  return 1;
}

static int builtin_get_theme_path(lua_State* L)
{
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));
  char* path;
  context_acquire_theme_path(context, &path);
  lua_pushstring(L, path);
  g_free(path);
  return 1;
}

int builtin_register_module(lua_State* L)
{
  const luaL_Reg table[] = {
    { "get"                 , builtin_get                  },
    { "set"                 , builtin_set                  },
    { "get_config"          , builtin_get_config           },
    { "set_config"          , builtin_set_config           },
    { "reset_config"        , builtin_reset_config         },
    { "set_keymap"          , builtin_set_keymap           },
    { "unset_keymap"        , builtin_unset_keymap         },
    { "set_keymaps"         , builtin_set_keymaps          },
    { "reset_keymaps"       , builtin_reset_keymaps        },
    { "set_hook"            , builtin_set_hook             },
    { "set_hooks"           , builtin_set_hooks            },
    { "reload"              , builtin_reload               },
    { "reload_theme"        , builtin_reload_theme         },
    { "apply"               , builtin_apply                },
    { "send_key"            , builtin_send_key             },
    { "set_timeout"         , builtin_set_timeout          },
    { "clear_timeout"       , builtin_clear_timeout        },
    { "put"                 , builtin_put                  },
    { "bell"                , builtin_bell                 },
    { "open"                , builtin_open                 },
    { "notify"              , builtin_notify               },
    { "copy"                , builtin_copy                 },
    { "paste"               , builtin_paste                },
    { "increase_font_scale" , builtin_increase_font_scale  },
    { "decrease_font_scale" , builtin_decrease_font_scale  },
    { "reset_font_scale"    , builtin_reset_font_scale     },
    { "check_mod"           , builtin_check_mod            },
    { "color_to_rgba"       , builtin_color_to_rgba        },
    { "rgba_to_color"       , builtin_rgba_to_color        },
    { "rgb_to_hex"          , builtin_rgb_to_hex           },
    { "get_monitor_model"   , builtin_get_monitor_model    },
    { "get_config_path"     , builtin_get_config_path      },
    { "get_theme_path"      , builtin_get_theme_path       },
    { "get_version"         , builtin_get_version          },
    { NULL, NULL },
  };
  Context* context = (Context*)lua_touserdata(L, lua_upvalueindex(1));

  luaL_newlibtable(L, table);
  lua_pushlightuserdata(L, context);
  luaL_setfuncs(L, table, 1);
  return 1;
}
