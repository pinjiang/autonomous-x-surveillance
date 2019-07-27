#include <gst/sdp/sdp.h>
#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

#include "utils.h"
#include "defs.h"
#include "common.h"
#include "gst_webrtc.h"

#define STUN_SERVER_PREFIX " stun-server=stun://"

static gint values = 0;

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void handle_media_stream (GstPad * pad, GstElement * pipe, const char * convert_name, const char * sink_name) {
  GstPad *qpad;
  GstElement *q, *conv, *resample, *sink;
  GstPadLinkReturn ret;

  g_print ("Trying to handle stream with %s ! %s", convert_name, sink_name);

  q = gst_element_factory_make ("queue", NULL);
  g_assert_nonnull (q);
  conv = gst_element_factory_make (convert_name, NULL);
  g_assert_nonnull (conv);
  sink = gst_element_factory_make (sink_name, NULL);
  g_assert_nonnull (sink);

  if (g_strcmp0 (convert_name, "audioconvert") == 0) {
    /* Might also need to resample, so add it just in case.
     * Will be a no-op if it's not required. */
    resample = gst_element_factory_make ("audioresample", NULL);
    g_assert_nonnull (resample);
    gst_bin_add_many (GST_BIN (pipe), q, conv, resample, sink, NULL);
    gst_element_sync_state_with_parent (q);
    gst_element_sync_state_with_parent (conv);
    gst_element_sync_state_with_parent (resample);
    gst_element_sync_state_with_parent (sink);
    gst_element_link_many (q, conv, resample, sink, NULL);
  } else {
    gst_bin_add_many (GST_BIN (pipe), q, conv, sink, NULL);
    gst_element_sync_state_with_parent (q);
    gst_element_sync_state_with_parent (conv);
    gst_element_sync_state_with_parent (sink);
    gst_element_link_many (q, conv, sink, NULL);
  }

  qpad = gst_element_get_static_pad (q, "sink");

  ret = gst_pad_link (pad, qpad);
  g_assert_cmphex (ret, ==, GST_PAD_LINK_OK);
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_incoming_decodebin_stream (GstElement * decodebin, GstPad * pad, GstElement * pipe) {
  GstCaps *caps;
  const gchar *name;
  const gchar *capsString;
  const gchar *structureString;
  const GValue *formatValue;

  if (!gst_pad_has_current_caps (pad)) {
    g_printerr ("Pad '%s' has no caps, can't do anything, ignoring\n", GST_PAD_NAME (pad));
    return;
  }

  caps = gst_pad_get_current_caps (pad);
  name = gst_structure_get_name (gst_caps_get_structure (caps, 0));
  g_print ("Pad name: %s\n", name); 
  //add by liyujia
  capsString = gst_caps_to_string (caps);
  g_print ("caps string: %s\n", capsString);
  formatValue = gst_structure_get_value (gst_caps_get_structure (caps, 0), "width");
  values = g_value_get_int (formatValue);
  structureString = gst_structure_to_string (gst_caps_get_structure (caps, 0));
  g_print ("formate value: %d\n", values); 
  //add end

  if (g_str_has_prefix (name, "video")) {
    handle_media_stream (pad, pipe, "videoconvert", "autovideosink");
  } else if (g_str_has_prefix (name, "audio")) {
    handle_media_stream (pad, pipe, "audioconvert", "autoaudiosink");
  } else {
    g_printerr ("Unknown pad %s, ignoring", GST_PAD_NAME (pad));
  }
}


/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_incoming_stream (GstElement * webrtc, GstPad * pad, GstElement * pipe) {
  GstElement *decodebin;

  if (GST_PAD_DIRECTION (pad) != GST_PAD_SRC)
    return;

  decodebin = gst_element_factory_make ("decodebin", NULL);
  g_signal_connect (decodebin, "pad-added",
      G_CALLBACK (on_incoming_decodebin_stream), pipe);
  gst_bin_add (GST_BIN (pipe), decodebin);
  gst_element_sync_state_with_parent (decodebin);
  gst_element_link (webrtc, decodebin);
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void send_ice_candidate_message(GstElement * webrtc G_GNUC_UNUSED, guint mlineindex, gchar *cand, gpointer user_data G_GNUC_UNUSED) {
  gchar *text;
  JsonObject *candidate, *msg, *content;
  
  InstanceCtx* ctx = (InstanceCtx *)user_data;

  if ( ctx->app_state < PEER_CALL_NEGOTIATING) {
    cleanup_stream (ctx, "Can't send ICE, not in call", APP_STATE_ERROR);
    return;
  }
  
  content = json_object_new();
  json_object_set_string_member(content, "type", "ice");
  json_object_set_string_member(content, "index", ctx->index);
  candidate = json_object_new();
  json_object_set_string_member(candidate, "candidate", cand);
  json_object_set_int_member(candidate, "sdpMLineIndex", mlineindex);
  json_object_set_object_member (content, "candidateLine", candidate);

  msg = json_object_new ();
  json_object_set_string_member(msg, "direction", "p2p");
  json_object_set_int_member(msg, "seq", 1);
  json_object_set_string_member(msg, "from", ctx->session_p->id);
  json_object_set_string_member(msg, "role", "vehicle.video");
  json_object_set_string_member(msg, "to",   ctx->session_p->peer_id);
  json_object_set_object_member(msg, "content", content);
  text = get_string_from_json_object (msg);
  
  json_object_unref (msg);
  
  soup_websocket_connection_send_text (ctx->session_p->ws_conn, text);
  g_free (text);
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void send_sdp_offer(GstWebRTCSessionDescription * offer, InstanceCtx *ctx) {
  gchar *text;
  JsonObject *msg, *sdp, *content;

  if (ctx->app_state < PEER_CALL_NEGOTIATING) {
    cleanup_stream (ctx, "Can't send offer, not in call", APP_STATE_ERROR);
    return;
  }

  text = gst_sdp_message_as_text (offer->sdp);
  g_info ("Sending offer :\n%s\n", text);

  content = json_object_new ();
  json_object_set_string_member(content, "type",  "offer");
  json_object_set_string_member(content, "index", ctx->index);
  sdp = json_object_new ();
  json_object_set_string_member(content, "sdp", text);
  g_free (text);
  
  msg = json_object_new ();
  json_object_set_int_member(msg, "seq", 1);
  json_object_set_string_member(msg, "direction", "p2p");
  json_object_set_string_member(msg, "from", ctx->session_p->id);
  json_object_set_string_member(msg, "role", "vehicle.video");
  json_object_set_string_member(msg, "to",   ctx->session_p->peer_id);
  json_object_set_object_member(msg, "content", content);
  text = get_string_from_json_object (msg);
  
  json_object_unref (msg);
  soup_websocket_connection_send_text (ctx->session_p->ws_conn, text);
  g_free (text);
}

/*********************************************************************************************
 * Description : Offer created by our pipeline, to be sent to the peer                       *                                                        
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_offer_created(GstPromise * promise, gpointer user_data) {
  GstWebRTCSessionDescription *offer = NULL;
  const GstStructure *reply;
  
  InstanceCtx *ctx = (InstanceCtx *)user_data;

  g_assert_cmphex (ctx->app_state, ==, PEER_CALL_NEGOTIATING);
  g_assert_cmphex (gst_promise_wait(promise), ==, GST_PROMISE_RESULT_REPLIED);

  reply = gst_promise_get_reply (promise);
  gst_structure_get (reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, NULL);
  gst_promise_unref (promise);

  promise = gst_promise_new ();
  g_signal_emit_by_name (ctx->webrtc, "set-local-description", offer, promise);
  gst_promise_interrupt (promise);
  gst_promise_unref (promise);

  /* Send offer to peer */
  send_sdp_offer(offer, ctx);
  gst_webrtc_session_description_free (offer);
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_negotiation_needed(GstElement * element, gpointer user_data) {
  GstPromise  *promise;
  InstanceCtx *ctx = (InstanceCtx *)user_data;

  ctx->app_state = PEER_CALL_NEGOTIATING;
  promise = gst_promise_new_with_change_func(on_offer_created, user_data, NULL);;
  g_signal_emit_by_name (ctx->webrtc, "create-offer", NULL, promise);
}


/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gboolean start_pipeline(InstanceCtx *ctx, const char *stunserver) {
  GstStateChangeReturn ret;
  GError *error = NULL;
  
  // char* pipeline_str = g_strconcat("webrtcbin bundle-policy=max-bundle name=sendrecv ", STUN_SERVER_PREFIX, stunserver, " ", ctx->pipeline_str,"! sendrecv. ", NULL);

  char* pipeline_str = g_strconcat("webrtcbin bundle-policy=max-bundle name=sendrecv ", STUN_SERVER_PREFIX, stunserver, " ", 
      "videotestsrc is-live=true pattern=ball ! videoconvert ! queue ! vp9enc ! rtpvp9pay ! application/x-rtp,media=video,encoding-name=VP9,payload=96 ","! sendrecv. ",
      "videotestsrc is-live=true pattern=ball ! videoconvert ! queue ! vp9enc ! rtpvp9pay ! application/x-rtp,media=video,encoding-name=VP9,payload=96 ","! sendrecv. ", NULL);
  
  g_print("Full pipeline: %s\n", pipeline_str);
      
  ctx->pipe = gst_parse_launch (pipeline_str, &error);
  g_free(pipeline_str); 
   
  if (error) {
    g_printerr ("Failed to parse launch: %s\n", error->message);
    g_error_free (error);
    goto err;
  }
  
  ctx->webrtc = gst_bin_get_by_name (GST_BIN (ctx->pipe), "sendrecv");
  g_assert_nonnull (ctx->webrtc);
  
  /* This is the gstwebrtc entry point where we create the offer and so on. It
   * will be called when the pipeline goes to PLAYING. */
  g_signal_connect (ctx->webrtc, "on-negotiation-needed", G_CALLBACK (on_negotiation_needed), ctx);
  /* We need to transmit this ICE candidate to the browser via the websockets
   * signalling server. Incoming ice candidates from the browser need to be
   * added by us too, see on_server_message() */
  g_signal_connect (ctx->webrtc, "on-ice-candidate", G_CALLBACK (send_ice_candidate_message), ctx);
  /* Incoming streams will be exposed via this signal */
  g_signal_connect (ctx->webrtc, "pad-added", G_CALLBACK (on_incoming_stream), ctx->pipe);
  /* Lifetime is the same as the pipeline itself */
  gst_object_unref (ctx->webrtc);
  
  g_info ("Pipeline started\n");
  ret = gst_element_set_state (GST_ELEMENT (ctx->pipe), GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto err;

  return TRUE;
  
  err:
    if (ctx->pipe)
      g_clear_object (&ctx->pipe);
    if (ctx->webrtc)
      ctx->webrtc = NULL;
    return FALSE;
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gboolean start_pipelines(InstanceCtx *ctx, const char *pipeline_str) {
  GstStateChangeReturn ret;
  GError *error = NULL;

  g_print("Full pipeline 2: %s\n", pipeline_str);

  ctx->pipe = gst_parse_launch (pipeline_str, &error);

  if (error) {
    g_printerr ("Failed to parse launch: %s\n", error->message);
    g_error_free (error);
    goto err;
  }

  ctx->webrtc = gst_bin_get_by_name (GST_BIN (ctx->pipe), "sendrecv");
  g_assert_nonnull (ctx->webrtc);

  /* This is the gstwebrtc entry point where we create the offer and so on. It
   * will be called when the pipeline goes to PLAYING. */
  g_signal_connect (ctx->webrtc, "on-negotiation-needed", G_CALLBACK (on_negotiation_needed), ctx);
  /* We need to transmit this ICE candidate to the browser via the websockets
   * signalling server. Incoming ice candidates from the browser need to be
   * added by us too, see on_server_message() */
  g_signal_connect (ctx->webrtc, "on-ice-candidate", G_CALLBACK (send_ice_candidate_message), ctx);
  /* Incoming streams will be exposed via this signal */
  g_signal_connect (ctx->webrtc, "pad-added", G_CALLBACK (on_incoming_stream), ctx->pipe);
  /* Lifetime is the same as the pipeline itself */
  gst_object_unref (ctx->webrtc);

  g_info ("Pipeline started\n");
  ret = gst_element_set_state (GST_ELEMENT (ctx->pipe), GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto err;

  return TRUE;

  err:
    if (ctx->pipe)
      g_clear_object (&ctx->pipe);
    if (ctx->webrtc)
      ctx->webrtc = NULL;
    return FALSE;
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
char * build_pipeline(ApplicationCtx * app_ctx_p, JsonArray *streams, const char *stunserver) {
  GstStateChangeReturn ret;
  GError *error = NULL;
  InstanceCtx *ctx;
  gchar *pipeline_str;
  const gchar *name;
  gchar *tmp_str = g_strconcat("webrtcbin bundle-policy=max-bundle name=sendrecv ", STUN_SERVER_PREFIX, stunserver, " ", NULL);

  for(unsigned i=0; i< json_array_get_length(streams); i++) {
    name = json_array_get_string_element(streams, i);
    ctx = find_ctx(app_ctx_p, name);
    if( ctx == NULL )
      continue;
    pipeline_str = g_strconcat(tmp_str, ctx->pipeline_str, "! sendrecv. ", NULL);
    g_free(tmp_str);
    tmp_str = pipeline_str;
  }
  
  g_info("Full pipeline : %s\n", pipeline_str);

  return pipeline_str;
}



