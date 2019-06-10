#ifndef COMMON_H
#define COMMON_H

#include <string.h>
#include <glib-object.h>
#include <gobject/gvaluetypes.h>
#include <json-glib/json-glib.h>
#include <gst/gst.h>
#include <libsoup/soup.h>

enum AppState {
  APP_STATE_UNKNOWN = 0,
  APP_STATE_ERROR = 1, /* generic error */
  SERVER_CONNECTING = 1000,
  SERVER_CONNECTION_ERROR,
  SERVER_CONNECTED, /* Ready to register */
  SERVER_REGISTERING = 2000,
  SERVER_REGISTRATION_ERROR,
  SERVER_REGISTERED, /* Ready to call a peer */
  SERVER_CLOSED, /* server connection closed by us or the server */
  PEER_CONNECTING = 3000,
  PEER_CONNECTION_ERROR,
  PEER_CONNECTED,
  PEER_CALL_NEGOTIATING = 4000,
  PEER_CALL_STARTED,
  PEER_CALL_STOPPING,
  PEER_CALL_STOPPED,
  PEER_CALL_ERROR,
};

enum InstanceState {
  _STATE_UNKNOWN = 0,
  _STATE_ERROR = 1,      /* generic error */
  _CALL_NEGOTIATING = 4000,
  _CALL_STARTED,
  _CALL_STOPPING,
  _CALL_STOPPED,
  _CALL_ERROR,
};

typedef struct {
  enum AppState app_state;
  // enum InstanceState state;
  gchar* index;
  gchar* pipeline_str;
  GstElement *pipe;
  GstElement *webrtc;
} InstanceCtx;

typedef struct {
  enum AppState app_state;
  GHashTable* inst_tbl;
  GMainLoop * loop;
  SoupWebsocketConnection *ws_conn;
} ApplicationCtx;

gchar* get_string_from_json_object (JsonObject * object);

guint hash_func(gconstpointer key);

gboolean key_equal_func(gconstpointer a, gconstpointer b);

InstanceCtx* find_ctx (const gchar* name);

gboolean close_instance(InstanceCtx* ctx);

void handle_errors (enum AppState app_state);

gboolean cleanup_and_quit_loop (const gchar * msg, enum AppState state);

#endif


