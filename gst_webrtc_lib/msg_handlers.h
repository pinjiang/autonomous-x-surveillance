#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

// #include <json-glib/json-glib.h>
// #include "defs.h"

void handle_p2p_msg(ApplicationCtx *app_ctx_p, const gchar *from, JsonObject *content);

void handle_s2c_msg(ApplicationCtx *app_ctx_p, const gchar *from, JsonObject *content);

#endif

