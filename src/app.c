/**
 * tym.c
 *
 * Copyright (c) 2017 endaaman
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "tym.h"


static bool on_vte_key_press(GtkWidget* widget, GdkEventKey* event, void* user_data)
{
  UNUSED(widget);
  Context* context = (Context*)user_data;

  unsigned mod = event->state & gtk_accelerator_get_default_mod_mask();
  unsigned key = gdk_keyval_to_lower(event->keyval);

  if (context_perform_keymap(context, key, mod)) {
    return true;
  }
  return false;
}

static void on_vte_child_exited(VteTerminal* vte, int status, void* user_data)
{
  UNUSED(vte);
  UNUSED(status);
  Context* context = (Context*)user_data;
  g_application_quit(G_APPLICATION(context->app));
}

static void on_vte_title_changed(VteTerminal* vte, void* user_data)
{
  UNUSED(vte);
  Context* context = (Context*)user_data;

  GtkWindow* window = context_get_window(context);
  const char* title = vte_terminal_get_window_title(context_get_vte(context));
  char* next_title = NULL;
  bool result = hook_perform_title(context->hook, context->lua, title, &next_title);
  if (result) {
    if (next_title) {
      gtk_window_set_title(window, next_title);
    }
    return;
  }
  if (title) {
    gtk_window_set_title(window, title);
  }
}

static void on_vte_bell(VteTerminal* vte, void* user_data)
{
  UNUSED(vte);
  Context* context = (Context*)user_data;

  if (hook_perform_bell(context->hook, context->lua)) {
    return;
  }
  GtkWindow* window = context_get_window(context);
  if (!gtk_window_is_active(window)) {
    gtk_window_set_urgency_hint(window, true);
  }
}

static bool on_vte_click(VteTerminal* vte, GdkEvent* event, void* user_data)
{
  UNUSED(vte);
  UNUSED(event);
  UNUSED(user_data);
  return false;
}

#ifdef TYM_USE_VTE_SPAWN_ASYNC
static void on_vte_spawn(VteTerminal* vte, GPid pid, GError* error, void* user_data)
{
  UNUSED(vte);
  UNUSED(pid);
  Context* context = (Context*)user_data;
  if (error) {
    g_error("%s", error->message);
    g_application_quit(context->app);
    return;
  }
}
#endif

static bool on_window_focus_in(GtkWindow* window, GdkEvent* event, void* user_data)
{
  UNUSED(event);

  Context* context = (Context*)user_data;
  gtk_window_set_urgency_hint(window, false);
  hook_perform_activated(context->hook, context->lua);
  return false;
}

static bool on_window_focus_out(GtkWindow* window, GdkEvent* event, void* user_data)
{
  UNUSED(window);
  UNUSED(event);

  Context* context = (Context*)user_data;
  hook_perform_deactivated(context->hook, context->lua);
  return false;
}

void on_dbus_signal(
  GDBusConnection* conn,
  const char* sender_name,
  const char* object_path,
  const char* interface_name,
  const char* signal_name,
  GVariant* parameters,
  void* user_data)
{
  UNUSED(conn);
  UNUSED(sender_name);
  UNUSED(object_path);
  UNUSED(interface_name);
  context_handle_signal((Context*)user_data, signal_name, parameters);
}

int on_commnad_line(GApplication* app, GApplicationCommandLine* cli, void* user_data)
{
  df();
  UNUSED(cli);
  Context* context = (Context*)user_data;

  GVariantDict* options = g_application_command_line_get_options_dict(cli);
  option_set_values(context->option, options);
  bool version = option_get_version(context->option);
  if (version) {
    g_print("version %s (rev.%s)\n", PACKAGE_VERSION, BUILD_REV);
    return 0;
  }
  char* signal = option_get_signal(context->option);
  if (signal) {
    const char* path = g_application_get_dbus_object_path(app);
    dd("DBus is active: %s", path);
    GDBusConnection* conn = g_application_get_dbus_connection(app);
    GError* error = NULL;
    g_dbus_connection_emit_signal(conn, NULL, path, TYM_APP_ID, signal, NULL, &error);
    g_message("Signal `%s` has been sent.\n", signal);
    if (error) {
      g_error("%s", error->message);
      g_error_free(error);
    }
    return 0;
  }
  g_application_activate(app);
  return 0;
}

void on_activate(GApplication* app, void* user_data)
{
  df();
  GtkWindow* w = gtk_application_get_active_window(GTK_APPLICATION(app));
  if (w) {
    gtk_window_present(w);
    return;
  }

  Context* context = (Context*)user_data;
  context_load_config(context);
  context_load_theme(context);
  context_load_device(context);
  context_build_layout(context);

  VteTerminal* vte = context_get_vte(context);
  GtkWindow* window = context_get_window(context);

  g_signal_connect(vte, "key-press-event", G_CALLBACK(on_vte_key_press), context);
  g_signal_connect(vte, "child-exited", G_CALLBACK(on_vte_child_exited), context);
  g_signal_connect(vte, "window-title-changed", G_CALLBACK(on_vte_title_changed), context);
  g_signal_connect(vte, "bell", G_CALLBACK(on_vte_bell), context);
  g_signal_connect(vte, "button-press-event", G_CALLBACK(on_vte_click), context);
  g_signal_connect(window, "focus-in-event", G_CALLBACK(on_window_focus_in), context);
  g_signal_connect(window, "focus-out-event", G_CALLBACK(on_window_focus_out), context);

  const char* path = g_application_get_dbus_object_path(app);
  dd("DBus is active: %s", path);
  GDBusConnection* conn = g_application_get_dbus_connection(app);
  g_dbus_connection_signal_subscribe(
    conn,
    NULL,       // sender
    TYM_APP_ID, // interface_name
    NULL,       // member
    path,       // object_path
    NULL,       // arg0
    G_DBUS_SIGNAL_FLAGS_NONE,
    on_dbus_signal,
    context,
    NULL        // user data free func
  );
  context_apply_config(context);
  context_apply_theme(context);

  GError* error = NULL;
  char** argv;
  char* line = config_get_str(context->config, "shell");
  g_shell_parse_argv(line, NULL, &argv, &error);
  if (error) {
    g_error("%s", error->message);
    g_error_free(error);
    g_application_quit(app);
    return;
  }
  char** env = g_get_environ();
  env = g_environ_setenv(env, "TERM", config_get_str(context->config, "term"), true);

#ifdef TYM_USE_VTE_SPAWN_ASYNC
  vte_terminal_spawn_async(
    vte,                 // terminal
    VTE_PTY_DEFAULT,     // pty flag
    NULL,                // working directory
    argv,                // argv
    env,                 // envv
    G_SPAWN_SEARCH_PATH, // spawn_flags
    NULL,                // child_setup
    NULL,                // child_setup_data
    NULL,                // child_setup_data_destroy
    1000,                // timeout
    NULL,                // cancel callback
    on_vte_spawn,        // callback
    context              // user_data
  );
#else
  GPid child_pid;
  vte_terminal_spawn_sync(
    vte,
    VTE_PTY_DEFAULT,
    NULL,
    argv,
    env,
    G_SPAWN_SEARCH_PATH,
    NULL,
    NULL,
    &child_pid,
    NULL,
    &error
  );

  if (error) {
    g_strfreev(env);
    g_strfreev(argv);
    g_error("%s", error->message);
    g_error_free(error);
    g_application_quit(app);
    return;
  }
#endif

  g_strfreev(env);
  g_strfreev(argv);
  gtk_widget_grab_focus(GTK_WIDGET(vte));
  gtk_widget_show_all(GTK_WIDGET(window));
}
