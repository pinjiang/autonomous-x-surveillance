#ifndef DEFS_H
#define DEFS_H

#include <gst/gst.h>
#include <gmodule.h>
#include <libsoup/soup.h>
// #include "utils.h"

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

enum SessionState {
  SESSION_STATE_UNKNOWN = 0,
  SESSION_STATE_ERROR = 1, /* generic error */
  SESSION_PEER_CONNECTING = 3000,
  SESSION_PEER_CONNECTION_ERROR,
  SESSION_PEER_CONNECTED,
  SESSION_PEER_CALL_NEGOTIATING = 4000,
  SESSION_PEER_CALL_STARTED,
  SESSION_PEER_CALL_STOPPING,
  SESSION_PEER_CALL_STOPPED,
  SESSION_PEER_CALL_ERROR,
};

enum StreamState {
  STREAM_STATE_UNKNOWN = 0,
  STREAM_STATE_ERROR = 1,      /* generic error */
  STREAM_ATTACHED = 4000,
  STREAM_UNATTACHED,
};

typedef struct {
  enum SessionState       state;
  gchar                   *id;
  gchar                   *peer_id;
  SoupWebsocketConnection *ws_conn;
  gchar                   *pipeline_str;
  GstElement              *pipe;
  GstElement              *webrtc;
} SessionCtx;

typedef struct {
  enum AppState           state;
  GMainLoop               *loop;
  SoupWebsocketConnection *ws_conn;
  GHashTable              *inst_tbl;
  gchar                   *id;
  gchar                   *stunserver;
  SessionCtx              *session_p;
} ApplicationCtx;

typedef struct {
  enum AppState           app_state;
  // enum InstanceState state;
  gchar                   *index;
  gchar                   *pipeline_str;
  GstElement              *pipe;
  GstElement              *webrtc;
  SessionCtx              *session_p;     /* Pointer to Parent Application Context */
} InstanceCtx;

// extern GMainLoop *loop;
// extern SoupWebsocketConnection *ws_conn;
// extern ApplicationCtx g_app_ctx;

// extern gchar *g_id;
// extern gchar *g_peer_id;
// extern gchar *g_stunserver;

#endif
