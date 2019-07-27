#ifndef COMMON_H
#define COMMON_H

#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>
// #include <gobject/gvaluetypes.h>
#include <json-glib/json-glib.h>

InstanceCtx* find_ctx (const ApplicationCtx *app_ctx_p, const gchar* name);

gboolean close_instance(InstanceCtx* ctx);

void handle_errors (enum AppState app_state);

gboolean cleanup_stream  (InstanceCtx * ctx_p, const gchar *msg, enum AppState state);

gboolean cleanup_session (SessionCtx * session_p, const gchar *msg, enum AppState state);

gboolean cleanup_and_quit_loop (ApplicationCtx * app_p, const gchar *msg, enum AppState state);

#endif
