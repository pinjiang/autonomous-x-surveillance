#include "defs.h"
#include "common.h"

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
InstanceCtx* find_ctx (const ApplicationCtx *app_ctx_p, const gchar* name) {
  InstanceCtx* ctx;
  gconstpointer key = name;
  ctx = (InstanceCtx*)g_hash_table_lookup(app_ctx_p->inst_tbl, key);
  if ( ctx == NULL )
    g_warning("Failed to find gst-pipeline/webrtc context with key:%s\n", name);

  return ctx;
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gboolean close_instance(InstanceCtx* ctx) {
  gst_element_set_state(ctx->pipe, GST_STATE_NULL); 
  return TRUE;
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
void handle_errors (enum AppState app_state) {
  switch (app_state) {
    case SERVER_CONNECTING:
      app_state = SERVER_CONNECTION_ERROR;
      break;
    case SERVER_REGISTERING:
      app_state = SERVER_REGISTRATION_ERROR;
      break;
    case PEER_CONNECTING:
      app_state = PEER_CONNECTION_ERROR;
      break;
    case PEER_CONNECTED:
    default:
      app_state = APP_STATE_ERROR;
  }
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gboolean cleanup_stream (InstanceCtx * ctx_p, const gchar *msg, enum AppState state) {
  if (msg)
    g_printerr ("%s\n", msg);

  if (state > 0) {
    // do something with key and value
    ctx_p->app_state = state;
    gst_element_set_state (GST_ELEMENT (ctx_p->pipe), GST_STATE_NULL);
    gst_object_unref (ctx_p->pipe);
    g_print ("Pipeline stopped\n");
  }

  return G_SOURCE_REMOVE;
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gboolean cleanup_session (SessionCtx *session_p, const gchar *msg, enum AppState state) {
  if (msg)
    g_printerr ("%s\n", msg);

  return G_SOURCE_REMOVE;
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gboolean cleanup_and_quit_loop (ApplicationCtx *app_p, const gchar *msg, enum AppState state) {
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, app_p->inst_tbl);
  
  if (msg)
    g_printerr ("%s\n", msg);

#if 0
  if (state > 0) {
    while (g_hash_table_iter_next (&iter, &key, &value)) {
      // do something with key and value
      InstanceCtx* ctx = (InstanceCtx * )value;
      ctx->app_state = state;
      gst_element_set_state (GST_ELEMENT (ctx->pipe), GST_STATE_NULL);
      gst_object_unref (ctx->pipe);
      g_print ("Pipeline stopped\n");
    }
  }

  if (ws_conn) {
    if (soup_websocket_connection_get_state (ws_conn) == SOUP_WEBSOCKET_STATE_OPEN)
      soup_websocket_connection_close (ws_conn, 1000, "");  /* This will call us again */
    else
      g_object_unref (ws_conn);
  }

  if (loop) {
    g_main_loop_quit (loop);
    loop = NULL;
  }
#endif

  /* To allow usage as a GSourceFunc */
  return G_SOURCE_REMOVE;
}
