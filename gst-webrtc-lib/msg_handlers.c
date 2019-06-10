#include <gst/gst.h>
#include <gst/sdp/sdp.h>
#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

#include "defs.h"
#include "utils.h"
#include "gst_webrtc.h"

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void handle_reg_rsp(const ApplicationCtx * app_ctx_p, const gchar *sctype, JsonObject *content) {
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, app_ctx_p->inst_tbl);
  const gchar* result, *reason;
  InstanceCtx* ctx;
  
  /* Check result of content */
  if (!json_object_has_member (content, "result")) {
    g_warning("message content without 'result' field");
    goto err;
  }
  result = json_object_get_string_member (content, "result");
  
  if (strcmp (result, "success") == 0) {

    while (g_hash_table_iter_next (&iter, &key, &value)) {
      ctx = (InstanceCtx * )value;
      if ( ctx->app_state != SERVER_REGISTERING) {
        g_warning ("Receive register success when not registering");
        goto err;
      }
      ctx->app_state = SERVER_REGISTERED;
    }

  } else if (strcmp (result, "failed") == 0) {
    reason = json_object_get_string_member (content, "reason");
    g_info ("register failed reason: %s\n", reason);

    while (g_hash_table_iter_next (&iter, &key, &value)) {
      ctx = (InstanceCtx * )value;
      handle_errors(ctx->app_state);
    }

    g_warning ("register failed");
    goto err;
  } else {
    g_warning("unknown result %s in content", result);
    goto err;
  }
  
err: 
  return;
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void handle_call_req(const ApplicationCtx * app_ctx_p, const gchar* sctype, JsonObject *content) {
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, app_ctx_p->inst_tbl);

  g_info("Receive call request and set all instance to PEER CONNECTED\n");
  
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    InstanceCtx* ctx = (InstanceCtx * )value;
    ctx->app_state = PEER_CONNECTED;
  }     
  
  return;

}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void handle_call_rsp(const ApplicationCtx * app_ctx_p, const gchar* sctype, JsonObject *content) {
  const gchar* result;
  
  /* Check result of content */
  if (!json_object_has_member (content, "result")) {
    g_warning("message without 'result' field");
    goto err;
  }
  result = json_object_get_string_member (content, "result");
  g_info ("Call response result: %s\n", result);
  
err:
  return;
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
void handle_s2c_msg( ApplicationCtx * app_ctx_p, const gchar *from, JsonObject *content) {
  const gchar *sctype;

  /* Check type of content */
  if (!json_object_has_member (content, "type")) {
    g_warning ("message without 'type' field");
    goto out;
  }
  sctype = json_object_get_string_member (content, "type");

  if (strcmp (sctype, "register_response") == 0) {
    handle_reg_rsp(app_ctx_p, sctype, content);
  } else if (strcmp (sctype, "call") == 0) {
    handle_call_req(app_ctx_p, sctype, content);
  } else if (strcmp (sctype, "call_response") == 0) {
    handle_call_rsp(app_ctx_p, sctype, content);
  } else {
    g_warning ("invalid message type %s", sctype);
    goto out;
  }

out:
  return;
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
void handle_p2p_msg(ApplicationCtx *app_ctx_p, const gchar *from, JsonObject *content) {
  JsonObject *child;
  /* Check content of JSON message */
  int ret;
  GstSDPMessage *sdp;
  const gchar *text, *type, *name;
  JsonArray* streams;
  InstanceCtx* ctx;
  GstWebRTCSessionDescription *answer;
    
  if (!json_object_has_member (content, "type")) {
    g_warning ("received content without 'type'");
    goto out;
  }
  type = json_object_get_string_member (content, "type");
  
  // check offer is null or not
  if (strcmp(type, "answer") == 0 ) {
    if (!json_object_has_member (content, "index")) {
      g_warning ("received content without 'index'");
      goto out;
    }
    name = json_object_get_string_member (content, "index");

    text = json_object_get_string_member (content, "sdp");
    g_info ("Received answer SDP :%s\n", text); 
    ret = gst_sdp_message_new (&sdp);
    g_assert_cmphex (ret, ==, GST_SDP_OK);
    ret = gst_sdp_message_parse_buffer ((guint8 *) text, strlen (text), sdp);
    g_assert_cmphex (ret, ==, GST_SDP_OK);
    answer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_ANSWER, sdp);
    g_assert_nonnull (answer);
    
    if( (ctx = find_ctx(app_ctx_p, name)) == NULL )
      goto out;
        
    GstPromise *promise = gst_promise_new ();
    g_assert_nonnull (ctx->webrtc);
    g_signal_emit_by_name (ctx->webrtc, "set-remote-description", answer, promise);   // 3: Set remote description on our pipeline
    ctx->app_state = PEER_CALL_STARTED;
      
    gst_promise_interrupt (promise);
    gst_promise_unref (promise);
    g_assert_nonnull(ctx->webrtc);
    
  } else if( strcmp(type, "open") == 0 ) {

    app_ctx_p->session_p = g_new0(SessionCtx, 1);
    app_ctx_p->session_p->ws_conn = app_ctx_p->ws_conn;
    app_ctx_p->session_p->id = app_ctx_p->id;
    app_ctx_p->session_p->peer_id = g_strdup(from);
    
    if (!json_object_has_member (content, "streams"))
      goto out;
    streams = json_object_get_array_member (content, "streams");

    char *pipeline_str = build_pipeline(app_ctx_p, streams, app_ctx_p->stunserver);

    g_print("%s\n", pipeline_str);

#if 0
    for(int i=0; i< json_array_get_length(streams); i++) {
      name = json_array_get_string_element(streams, i);
      ctx = find_ctx(app_ctx_p, name);
      if( ctx == NULL ) 
        continue;

      // ctx->session_p = app_ctx_p->session_p;
      // ctx->app_state = PEER_CONNECTED;
      /* if ( !start_pipeline(ctx, app_ctx_p->stunserver) )
        cleanup_stream (ctx, "ERROR: failed to start pipeline", 0); */
    }
#endif

    name = json_array_get_string_element(streams, 0);
    ctx->session_p = app_ctx_p->session_p;
    ctx->app_state = PEER_CONNECTED;
    ctx = find_ctx(app_ctx_p, name);
    if ( !start_pipelines(ctx, pipeline_str) )
      cleanup_stream (ctx, "ERROR: failed to start pipeline", 0);
    g_free(pipeline_str);

  } else if (strcmp(type, "close") == 0 )  {
    if (!json_object_has_member (content, "streams")) {
      goto out;
    }
    streams = json_object_get_array_member (content, "streams");
    
    for(int i=0; i<json_array_get_length(streams); i++) {
      const char* name = json_array_get_string_element(streams, i);
      if( (ctx = find_ctx(app_ctx_p, name)) == NULL )
        continue;

      close_instance(ctx);
    }
  } else if (strcmp(type, "ice") == 0 )  {
    /* ICE candidate received from peer, add it */
    const gchar *candidate;
    gint sdpmlineindex;
    if (!json_object_has_member (content, "index")) {
       g_info ("received content without 'index'");
       goto out;
    }
    name = json_object_get_string_member (content, "index");

    child = json_object_get_object_member (content, "candidateLine");
    if (!json_object_has_member (child, "candidate")) {
       g_info ("received content without 'candidate'");
       goto out;
    }
    candidate = json_object_get_string_member (child, "candidate");
    sdpmlineindex = json_object_get_int_member (child, "sdpMLineIndex");

    gconstpointer key = name;
    ctx = (InstanceCtx*)g_hash_table_lookup(app_ctx_p->inst_tbl, key);
    if ( ctx == NULL ) {
      g_warning ("failed to find context with key:%s\n", (char *)key);
      goto out;
    }
    g_signal_emit_by_name (ctx->webrtc, "add-ice-candidate", sdpmlineindex, candidate);  // Add ice candidate sent by remote peer
    g_info ("remote ice candidate added %s!\n", candidate);

  } else {
    g_warning ("invalid 'type' %s in content", type);
    goto out;
  }
  
 out:
  return;
}

