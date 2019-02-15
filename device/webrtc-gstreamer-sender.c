/*
 * Demo gstreamer app for negotiating and streaming a sendrecv webrtc stream
 * with a browser JS app.
 *
 * gcc webrtc-gstreamer-sender.c $(pkg-config --cflags --libs gstreamer-webrtc-1.0 gstreamer-sdp-1.0 libsoup-2.4 json-glib-1.0) -o webrtc-gstreamer-sender
 *
 * Author: Nirbheek Chauhan <nirbheek@centricular.com>
 */

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <pthread.h> 
#include <glib-object.h>
#include <gobject/gvaluetypes.h>

#include "global.h"
#include "common.h"
#include "msg_handlers.h"

#define G_MESSAGES_PREFIXED "auto-x-survelliance"
#define G_LOG_DOMAIN "auto-x-survelliance"

static volatile gboolean g_running_flag = FALSE;

GMainLoop *loop;
// GHashTable* g_ctx_tbl;
SoupWebsocketConnection *ws_conn = NULL;

ApplicationCtx g_app_ctx = {APP_STATE_UNKNOWN, NULL, NULL, NULL};

static const gchar *g_config_file = NULL;
static const gchar *g_verbose = NULL;

static gboolean disable_ssl = TRUE;
static gchar *g_url;
gchar *g_id;
gchar *g_peer_id = "XXXX";
gchar *g_stunserver;

static GOptionEntry entries[] = {
  { "config",  0, 0, G_OPTION_ARG_STRING, &g_config_file, "Config File", NULL },
  { "verbose", 'v', 0, G_OPTION_ARG_STRING, &g_verbose, "Be verbose", NULL },
  { NULL },
};

static void _dummy(const gchar *log_domain,
                     GLogLevelFlags log_level,
                     const gchar *message,
                     gpointer user_data )

{
  /* Dummy does nothing */ 
  return ;      
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
void sigint_func(int sig) {
   g_running_flag = FALSE;
}


/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gboolean register_with_server () {
  gchar *text;
  JsonObject *msg, *content;
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, g_app_ctx.inst_tbl);

  if (soup_websocket_connection_get_state (ws_conn) !=
      SOUP_WEBSOCKET_STATE_OPEN)
    return FALSE;

  g_info("Registering id %s with server\n", g_id);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    // do something with key and value
    InstanceCtx* ctx = (InstanceCtx *)value;
    ctx->app_state = SERVER_REGISTERING;
  }

  /* Register with the server with a random integer id. Reply will be received
   * by on_server_message() */
  content = json_object_new();
  json_object_set_string_member (content, "type", "register");
  json_object_set_string_member (content, "uid", g_id);
  msg = json_object_new();
  json_object_set_string_member (msg, "direction", "cs");
  json_object_set_int_member (msg, "seq", 1);
  json_object_set_string_member (msg, "from", g_id);
  json_object_set_string_member (msg, "role", "vehicle.video");
  json_object_set_object_member (msg, "content", content);

  text = get_string_from_json_object (msg);
  json_object_unref (msg);

  soup_websocket_connection_send_text (ws_conn, text);
  g_free (text);
  return TRUE;
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_server_closed (SoupWebsocketConnection * conn G_GNUC_UNUSED, gpointer user_data G_GNUC_UNUSED) {
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, g_app_ctx.inst_tbl);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    // do something with key and value
    InstanceCtx* ctx = (InstanceCtx *)value;
    ctx->app_state = SERVER_CLOSED;
  }
  cleanup_and_quit_loop ("Server connection closed", 0);
}

/*********************************************************************************************
 * Description: One mega message handler for our asynchronous calling mechanism              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_server_message (SoupWebsocketConnection * conn, SoupWebsocketDataType type, GBytes * message, gpointer user_data) {
  gsize size;
  gchar *text, *data;

  switch (type) {
    case SOUP_WEBSOCKET_DATA_BINARY:
      g_printerr ("Received unknown binary message, ignoring\n");
      g_bytes_unref (message);
      return;
    case SOUP_WEBSOCKET_DATA_TEXT:
      data = g_bytes_unref_to_data (message, &size); /* Convert to NULL-terminated string */
      text = g_strndup (data, size);
      g_free (data);
      break;
    default:
      g_assert_not_reached ();
  }

  /* parse JSON messages */
  JsonNode *root;
  JsonObject *object, *content;
  const gchar *from = NULL;
  const gchar *direction;
    
  JsonParser *parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, text, -1, NULL)) {
    g_warning ("Unknown message '%s', ignoring", text);
    goto out;
  }
  root = json_parser_get_root (parser);
  if (!JSON_NODE_HOLDS_OBJECT (root)) {
    g_warning ("Unknown json message '%s', ignoring", text);
    goto out;
  }
  object = json_node_get_object (root);

  if (!json_object_has_member (object, "direction")) { /* Check direction of JSON message */
    g_warning ("received message without 'direction'");
    goto out;
  }
  direction = json_object_get_string_member (object, "direction");

#if 0
  if (!json_object_has_member (object, "to")) {  /* Check from field of JSON message */
    cleanup_and_quit_loop ("ERROR: received message without 'to'", 0);
    goto out;
  }
  to = json_object_get_string_member (object, "to");
#endif
  
  if (!json_object_has_member (object, "content")) {  /* Check content of JSON message */
    g_warning ("received message without 'content'");
    goto out;
  }
  content = json_object_get_object_member (object, "content");

  if (strcmp (direction, "p2p") == 0) {              /* Handle Peer to Peer Message */
    if (!json_object_has_member (object, "from")) {  /* Check from field of JSON message */
      g_warning ("received message without 'from'");
      goto out;
    } else {
      from = json_object_get_string_member (object, "from");
      handle_p2p_msg(from, content);
    }
  } else if (strcmp (direction, "sc") == 0) {        /* Handle Server to Client Message */
    handle_s2c_msg(from, content);
  } else {
    g_warning ("invalid direction");
    goto out;
  }

out:
  g_object_unref (parser);
  g_free (text);

}

/*********************************************************************************************
 * Description: Connect to the signalling server. This is the entrypoint for everything else *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_server_connected (SoupSession * session, GAsyncResult * res) {
  GError *error = NULL;
  GHashTableIter iter;
  gpointer key, value;
  
  g_hash_table_iter_init (&iter, g_app_ctx.inst_tbl);

  ws_conn = soup_session_websocket_connect_finish (session, res, &error);
  if (error) {
    cleanup_and_quit_loop (error->message, SERVER_CONNECTION_ERROR);
    g_error_free (error);
    return;
  }

  g_assert_nonnull (ws_conn);


  /* Update per gstreamer pipeline context */
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    InstanceCtx* ctx = (InstanceCtx *)value;
    ctx->app_state = SERVER_CONNECTED;
  }

  g_info ("Connected to signalling server\n");

  g_signal_connect (ws_conn, "closed",  G_CALLBACK (on_server_closed),  NULL);
  g_signal_connect (ws_conn, "message", G_CALLBACK (on_server_message), NULL);

  /* Register with the server so it knows about us and can accept commands */
  register_with_server();
}


/*********************************************************************************************
 * Description: Connect to the signalling server. This is the entrypoint for everything else *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void connect_to_websocket_server_async () {
  SoupLogger *logger;
  SoupMessage *message;
  SoupSession *session;
  const char *https_aliases[] = {"wss", NULL};
  GHashTableIter iter;
  gpointer key, value;
  
  g_hash_table_iter_init (&iter, g_app_ctx.inst_tbl);

  session = soup_session_new_with_options (SOUP_SESSION_SSL_STRICT, !disable_ssl,
      SOUP_SESSION_SSL_USE_SYSTEM_CA_FILE, TRUE,
      //SOUP_SESSION_SSL_CA_FILE, "/etc/ssl/certs/ca-bundle.crt",
      SOUP_SESSION_HTTPS_ALIASES, https_aliases, NULL);

  logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
  soup_session_add_feature (session, SOUP_SESSION_FEATURE (logger));
  g_object_unref (logger);

  message = soup_message_new (SOUP_METHOD_GET, g_url);

  g_info ("Connecting to server...\n");

  /* Once connected, we will register */
  soup_session_websocket_connect_async (session, message, NULL, NULL, NULL, (GAsyncReadyCallback) on_server_connected, NULL);
   
  /* Update per gstreamer pipeline context */ 
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    InstanceCtx* ctx = (InstanceCtx *)value;
    ctx->app_state = SERVER_CONNECTING;
  }

}

/*********************************************************************************************
 * Description: Connect to the signalling server. This is the entrypoint for everything else *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gboolean check_plugins (void) {
  int i;
  gboolean ret;
  GstPlugin *plugin;
  GstRegistry *registry;
  const gchar *needed[] = { "opus", "vpx", "nice", "webrtc", "dtls", "srtp",
      "rtpmanager", "videotestsrc", "audiotestsrc", NULL}; /* "omx", */

  registry = gst_registry_get ();
  ret = TRUE;
  for (i = 0; i < g_strv_length ((gchar **) needed); i++) {
    plugin = gst_registry_find_plugin (registry, needed[i]);
    if (!plugin) {
      g_print ("Required gstreamer plugin '%s' not found\n", needed[i]);
      ret = FALSE;
      continue;
    }
    gst_object_unref (plugin);
  }
  return ret;
}

/*********************************************************************************************
 * Description: Connect to the signalling server. This is the entrypoint for everything else *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gboolean read_config(const char* filename) {
  GError     *error;
  JsonParser *parser;
  JsonNode   *root;
  JsonObject *object;
  JsonObject *pipelines;
  
  int nums; 
  gpointer keys;
  
  parser = json_parser_new();
  
  error = NULL;
  json_parser_load_from_file (parser, filename, &error);
  if (error) {
      g_print ("Unable to parse `%s': %s\n", filename, error->message);
      g_error_free (error);
      g_object_unref (parser);
      goto err;
  }
  
  /* manipulate the object tree and then exit */
  root = json_parser_get_root (parser);
  
  if (!JSON_NODE_HOLDS_OBJECT (root)) {
    // g_printerr ("Unknown json message '%s', ignoring", text);
    // g_object_unref (parser);
    goto err;
  }
  object = json_node_get_object (root);

  if (!json_object_has_member (object, "Num of Cameras")) { 
    g_printerr ("ERROR: received message without 'Num of Cameras'");
    goto err;
  }
  nums = json_object_get_int_member (object, "Num of Cameras");
  
  if (!json_object_has_member (object, "Url")) { 
    g_printerr ("ERROR: received message without 'Url'");
    goto err;
  }
  g_url = (char *)g_malloc0(strlen(json_object_get_string_member (object, "Url")));
  strcpy(g_url, json_object_get_string_member (object, "Url"));
  
  if (!json_object_has_member (object, "Id")) {         
    g_printerr ("ERROR: received message without 'Id'");
    goto err;
  }
  g_id = (char *)g_malloc0(strlen(json_object_get_string_member (object, "Id")));
  strcpy(g_id, json_object_get_string_member (object, "Id"));
  
  if (!json_object_has_member (object, "Stun-Server")) { 
    g_printerr ("ERROR: received message without 'Stun-Server'");
    goto err;
  } 
  g_stunserver = (char *)g_malloc0(strlen(json_object_get_string_member (object, "Stun-Server")));
  strcpy(g_stunserver, json_object_get_string_member (object, "Stun-Server"));
  
  if (!json_object_has_member (object, "Camera Pipelines")) {
    g_printerr ("ERROR: received message without 'Camera Pipelines'");
    goto err;
  }
  pipelines = json_object_get_object_member(object, "Camera Pipelines");
  keys   = json_object_get_members (json_object_get_object_member(object, "Camera Pipelines")); 
  
  g_app_ctx.inst_tbl = g_hash_table_new (hash_func, key_equal_func);
    
  for (int i = 0; i < MIN(nums, g_slist_length(keys)); i++) {
    // InstanceCtx * ctx = g_malloc(sizeof(InstanceCtx));
    InstanceCtx* ctx = g_new0(InstanceCtx, 1);
    ctx->index = g_malloc0(strlen((const char *)g_slist_nth_data(keys, i)));
    strcpy(ctx->index, (const char *)g_slist_nth_data(keys, i));
    gpointer key = ctx->index;
    g_info("Read %s: %s", (char *)key, json_object_get_string_member(pipelines, (const char *)key) );  

    ctx->pipeline_str = g_malloc(strlen(json_object_get_string_member(pipelines, (const char *)key)));
    strcpy(ctx->pipeline_str, json_object_get_string_member(pipelines, (const char *)key));
    g_hash_table_insert(g_app_ctx.inst_tbl, key, ctx); 
  } 
  
  g_list_free(keys);
  g_object_unref (parser);
  
  return TRUE;

  err:
    g_object_unref (parser); 
    return FALSE;
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
int main (int argc, char *argv[]) {
  GOptionContext *context;
  GError *error = NULL;

  context = g_option_context_new ("- gstreamer webrtc recv demo");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, gst_init_get_option_group ());
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("Error initializing: %s\n", error->message);
    return -1;
  }

  /* Set dummy for all levels */
  g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK, _dummy, NULL);

  if(!g_verbose) {
    g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK, g_log_default_handler, NULL);
  } else if(!strncmp("vv", g_verbose, 2)) {   /* If -vv passed set to ONLY debug */
    g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,  g_log_default_handler, NULL);
  } else if(!strncmp("v", g_verbose, 1)) { /* If -v passed set to ONLY info */
    g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, g_log_default_handler, NULL);
  } else { /* For everything else, set to back to default*/
    g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK, g_log_default_handler, NULL);
  }
  
  if (!g_config_file ) {
    g_printerr ("--config and --peer-id is a required argument\n");
    g_printerr ("%s", g_option_context_get_help (context, TRUE, NULL));
    return -1;
  }
  
  if (!check_plugins ())
    return -1;
	
  if(!read_config(g_config_file)) {
    g_printerr("Error parsing JSON config file\n");
    return -1;
  }

  // signal(SIGINT, sigint_func);
  
  /* Disable ssl when running a localhost server, because
   * it's probably a test server with a self-signed certificate */
  {
    GstUri *uri = gst_uri_from_string (g_url);
    if (g_strcmp0 ("localhost", gst_uri_get_host (uri)) == 0 || 
        g_strcmp0 ("127.0.0.1", gst_uri_get_host (uri)) == 0)
      disable_ssl = TRUE;
    gst_uri_unref (uri);
  }

  {
    if (g_str_has_prefix(g_url, "ws://") )
      disable_ssl = TRUE;
  }

  loop = g_main_loop_new (NULL, FALSE);
  connect_to_websocket_server_async();
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
 
  return 0;
}
