#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

#include <glib-object.h>
#include <gobject/gvaluetypes.h>
#include <json-glib/json-glib.h>

void handle_p2p_msg(const gchar *from, JsonObject *content);

void handle_s2c_msg(const gchar *from, JsonObject *content);

#endif

