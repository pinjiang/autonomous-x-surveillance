#ifndef GLOBAL_H
#define GLOBAL_H

#include <gst/gst.h>
#include <gmodule.h>
#include <libsoup/soup.h>

#include "common.h"

extern GMainLoop *loop;
extern SoupWebsocketConnection *ws_conn;

extern ApplicationCtx g_app_ctx;

extern gchar *g_id;
extern gchar *g_peer_id;
extern gchar *g_stunserver;

#endif
