/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003, Ximian, Inc.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <glib/gstdio.h>

#include "media_server.h"
#include "utils.h"
#include "gst_rtsp.h"

static gboolean is_wss = FALSE; /* Temporary variable */

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void do_get (SoupServer *server, SoupMessage *msg, const char *path) {
  char *slash;
  GStatBuf st;

  if (msg->method == SOUP_METHOD_GET) {
    // SoupBuffer *buffer;
    SoupMessageHeadersIter iter;
    const char *name, *value;

    /* mapping = g_mapped_file_new (path, FALSE, NULL);
    if (!mapping) {
      soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
      return;
    } */

    soup_message_headers_append (msg->response_headers,"Access-Control-Allow-Origin", "*");

    /* buffer = soup_buffer_new_with_owner (g_mapped_file_get_contents (mapping),
				 g_mapped_file_get_length (mapping),
				 mapping, (GDestroyNotify)g_mapped_file_unref);
		
    soup_message_body_append_buffer (msg->response_body, buffer); */
			
    // soup_buffer_free (buffer);
  } else /* msg->method == SOUP_METHOD_HEAD */ {
    char *length;

    /* We could just use the same code for both GET and
     * HEAD (soup-message-server-io.c will fix things up).
     * But we'll optimize and avoid the extra I/O.
     */
    length = g_strdup_printf ("%lu", (gulong)st.st_size);
    soup_message_headers_append (msg->response_headers, "Content-Length", length);
    g_free (length);
  }
  soup_message_set_status (msg, SOUP_STATUS_OK);
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void do_post (SoupServer *server, SoupMessage *msg) {
#if 0
  SoupBuffer *buffer;
  gchar *text;
  JsonObject *json_obj;
  GError **error;
	
  json_obj = json_object_new ();
  json_object_set_string_member (json_obj, "code", "0");
  text = json_to_string(json_obj, true);
  json_object_unref (json_obj);
	
  buffer = soup_buffer_new_with_owner (text, strlen(text), text, (GDestroyNotify)g_free);
	
  soup_message_headers_append (msg->response_headers,"Access-Control-Allow-Origin", "*");
  soup_message_body_append_buffer (msg->response_body, buffer);
  soup_buffer_free (buffer);
	
  soup_message_set_status (msg, SOUP_STATUS_OK);
#endif

  if (msg->request_body->length) {
    g_print ("%s\n", msg->request_body->data);

    //===============add by liyujia, 20180905=================
    //now ,parse json data
    JsonObject *object = parse_json_object(msg->request_body->data);

#if 0
    if (json_object_has_member (object, "ConnectToServer")) {
      JsonObject *child;
      const gchar *server_url_, *terminal_id_;
      child = json_object_get_object_member (object, "ConnectToServer");
      if (json_object_has_member (child, "server_url") && json_object_has_member (child, "terminal_id")) {
        server_url_ = json_object_get_string_member (child, "server_url");
        terminal_id_ = json_object_get_string_member (child, "terminal_id");
      }
      g_print ("server_url = %s,  terminal_id = %s\n", server_url_, terminal_id_);

      // checkAndConnect(server_url_, terminal_id_);
    } else if (json_object_has_member (object, "Monitoring")) {
      JsonObject *child;
      const gchar *_event_, *_data_, *_peerId_;
      child = json_object_get_object_member (object, "Monitoring");

      if (json_object_has_member (child, "_event") && json_object_has_member (child, "_data") && json_object_has_member (child, "_peerId")) {
        _event_ = json_object_get_string_member (child, "_event");
        _data_ = json_object_get_string_member (child, "_data");
        _peerId_ = json_object_get_string_member (child, "_peerId");
      }
      g_print ("event = %s,  data = %s, peerId = %s\n", _event_, _data_, _peerId_);

      if (!setup_call (_peerId_)) {
        g_print ("Failed to setup call!\n");
        cleanup_and_quit_loop ("ERROR: Failed to setup call", PEER_CALL_ERROR);
      }

      } else {
        g_print ("do nothing!\n");
      }
      //add end
#endif
  }

}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void do_put (SoupServer *server, SoupMessage *msg, const char *path) {
  GStatBuf st;
  FILE *f;
  gboolean created = TRUE;

  if (g_stat (path, &st) != -1) {
    const char *match = soup_message_headers_get_one (msg->request_headers, "If-None-Match");
    if (match && !strcmp (match, "*")) {
      soup_message_set_status (msg, SOUP_STATUS_CONFLICT);
      return;
    }
    if (!g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
      soup_message_set_status (msg, SOUP_STATUS_FORBIDDEN);
      return;
    }
    created = FALSE;
  }

  f = fopen (path, "w");
  if (!f) {
    soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  fwrite (msg->request_body->data, 1, msg->request_body->length, f);
  fclose (f);
  soup_message_set_status (msg, created ? SOUP_STATUS_CREATED : SOUP_STATUS_OK);
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void http_callback (SoupServer *server, SoupMessage *msg,
		 const char *path, GHashTable *query,
		 SoupClientContext *context, gpointer data) {
  char *file_path;
  SoupMessageHeadersIter iter;
  const char *name, *value;

  g_info ("%s %s HTTP/1.%d\n", msg->method, path, soup_message_get_http_version (msg));
  soup_message_headers_iter_init (&iter, msg->request_headers);

  while (soup_message_headers_iter_next (&iter, &name, &value))
    g_info ("%s: %s\n", name, value);

  file_path = g_strdup_printf (".%s", path);
  
  if (msg->method == SOUP_METHOD_GET || msg->method == SOUP_METHOD_HEAD)
    do_get (server, msg, file_path);
  else if (msg->method == SOUP_METHOD_PUT)
    do_put (server, msg, file_path);
  else if (msg->method == SOUP_METHOD_POST)
    do_post(server, msg);
  else
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);

  g_free (file_path);
  g_info ("  -> %d %s\n\n", msg->status_code, msg->reason_phrase);
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_text_message (SoupWebsocketConnection *ws,
                 SoupWebsocketDataType type,
                 GBytes *message,
                 gpointer user_data) {
	GBytes **receive = user_data;

	g_assert_cmpint (type, ==, SOUP_WEBSOCKET_DATA_TEXT);
	g_assert (*receive == NULL);
	g_assert (message != NULL);

	*receive = g_bytes_ref (message);
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_binary_message (SoupWebsocketConnection *ws,
		   SoupWebsocketDataType type,
		   GBytes *message,
		   gpointer user_data) {
	GBytes **receive = user_data;

	g_assert_cmpint (type, ==, SOUP_WEBSOCKET_DATA_BINARY);
	g_assert (*receive == NULL);
	g_assert (message != NULL);

	*receive = g_bytes_ref (message);
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_message(SoupWebsocketConnection *conn, gint type, GBytes *message, gpointer data) {

  AppContext *app = data;
  RTSPServerInfo info;
 
  if (type == SOUP_WEBSOCKET_DATA_TEXT) {
    gsize sz;
    const gchar *ptr;

    ptr = g_bytes_get_data(message, &sz);
    g_debug("Received text data: %s", ptr);
    info.location = ptr;
    info.user_id = "admin";
    info.user_pwd = "vlab123!";
    
    start_pipeline(&info, app->loop);  
    // soup_websocket_connection_send_text(conn, (is_wss) ? "Hello Secure Websocket !" : "Hello Websocket !");
  }
  else if (type == SOUP_WEBSOCKET_DATA_BINARY) {
    g_info("Received binary data (not shown)");

  } else {
    g_info("Invalid data type: %d", type);
  }
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_close(SoupWebsocketConnection *conn, gpointer data) {
  soup_websocket_connection_close(conn, SOUP_WEBSOCKET_CLOSE_NORMAL, NULL);
  g_info("WebSocket connection closed\n");
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_connection(SoupSession *session, GAsyncResult *res, gpointer data) {
  SoupWebsocketConnection *conn;
  GError *error = NULL;

  conn = soup_session_websocket_connect_finish(session, res, &error);
  if (error) {
    g_print("Error: %s\n", error->message);
    g_error_free(error);
    return;
  }
  g_signal_connect(conn, "message", G_CALLBACK(on_message), NULL);
  g_signal_connect(conn, "closed",  G_CALLBACK(on_close),   NULL);

  soup_websocket_connection_send_text(conn, (is_wss) ? "Hello Secure Websocket !" : "Hello Websocket !");
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void websocket_callback (SoupServer *server, SoupWebsocketConnection *connection,
     const char *path, SoupClientContext *client, gpointer user_data ) {

  GBytes *received = NULL;
  g_info("websocket connected");

  AppContext *ctx = user_data;
  ctx->server = g_object_ref (connection);
  g_signal_connect (ctx->server, "message", G_CALLBACK (on_message), &received);

  return;
}


/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
SoupServer* start_server(const int port, const char *tls_cert_file, const char *tls_key_file, AppContext *app) {
  GSList *uris, *u;
  GTlsCertificate *cert;
  char *str;
  GError *error = NULL;
  SoupServer *server;

  if ( tls_cert_file && tls_key_file ) {
    cert = g_tls_certificate_new_from_files (tls_cert_file, tls_key_file, &error);
    if (error) {
      g_error ("Unable to create server: %s\n", error->message);
      return NULL;
    }
    server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "simple-httpd ",
      SOUP_SERVER_TLS_CERTIFICATE, cert, NULL);
    g_object_unref (cert);
    soup_server_listen_local (server, port, SOUP_SERVER_LISTEN_HTTPS, &error);
  } else {
    server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "simple-httpd ", NULL);
    soup_server_listen_local (server, port, 0, &error);
  }
  // soup_server_add_handler (server, "/play", http_callback, NULL, NULL);
  soup_server_add_websocket_handler(server, NULL, NULL, NULL,
                                   websocket_callback,
                                   app, NULL);

  uris = soup_server_get_uris (server);
  for (u = uris; u; u = u->next) {
    str = soup_uri_to_string (u->data, FALSE);
    g_info ("Listening on %s", str);
    g_free (str);
    soup_uri_free (u->data);
  }
  g_slist_free (uris);
  g_info ("Waiting for requests...");
  return server;
}
