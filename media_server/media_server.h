#ifndef MEDIA_SERVER_H
#define MEDIA_SERVER_H

#include <libsoup/soup.h>

typedef struct {
  GMainLoop *loop;
  volatile gboolean g_running_flag;
  
  // oupSession *session;
  // SoupMessage *msg;
  // SoupWebsocketConnection *client;
  // GError *client_error;

  SoupServer *soup_server;
  SoupWebsocketConnection *server;

  // gboolean no_server;
  // GIOStream *raw_server;
} AppContext;

SoupServer* start_server(const int port, const char *tls_cert_file, const char *tls_key_file, AppContext *app);

#endif
