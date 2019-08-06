#include <glib.h>
#include <gst/gst.h>

#include "gst_rtsp.h"

/* the destination machine to send RTCP to. This is the address of the sender and
 * is used to send back the RTCP reports of this receiver. If the data is sent
 * from another machine, change this address. */
#define DEST_HOST "127.0.0.1"
static guint g_ssrc = 0; /* copy of the receiver ssrc */
//RTCP 控制参数获取
#define RTCP_MAX_NUM		22
#define RTCP_LEY_LEN_MAX	64
typedef enum {
	eInvalid = -1, eBollean, eInt, eUint, eInt64, eUint64
}RtcpValueType;
typedef union {
	gboolean	value_bool;
	gint 		value_int;
	guint 		value_uint;
	gint64		value_int64;
	guint64		value_uint64;
}RtcpValue;
typedef struct {
	gchar 			key[RTCP_LEY_LEN_MAX];
	RtcpValueType	value_type;	//数值类型
	RtcpValue		value;		//数值结果
	gdouble			fk;			//数值系数
}RtcpParseInfo;
//外部使用参数
RtcpParseInfo g_rtcp_parameter[RTCP_MAX_NUM] = {
		{"ssrc",			 	eUint, 0, 1.0},
		{"received-bye",		eBollean, 0, 1.0},
		{"clock-rate", 			eInt, 0, 1.0},
		{"octets-received", 	eUint64, 0, 1.0},
		{"packets-received", 	eUint64, 0, 1.0},
		{"bitrate", 			eUint64, 0, 1.0},
		{"packets-lost", 		eInt, 0, 1.0},
		{"jitter", 				eUint, 0, 1.0},
		{"sent-pli-count", 		eUint, 0, 1.0},
		{"recv-pli-count", 		eUint, 0, 1.0},
		{"sent-fir-count", 		eUint, 0, 1.0},
		{"recv-fir-count", 		eUint, 0, 1.0},

		{"sr-ntptime", 			eUint, 0, 1.0},
		{"sr-rtptime",			eUint, 0, 1.0},
		{"sr-octet-count",		eUint, 0, 1.0},
		{"sr-packet-count",		eUint, 0, 1.0},

		{"sent-rb-fractionlost",eUint, 0, 1.0},
		{"sent-rb-packetslost",	eInt, 0, 1.0},
		{"sent-rb-exthighestseq",eUint, 0, 1.0},
		{"sent-rb-jitter",		eUint, 0, 1.0},
		{"sent-rb-lsr",			eUint, 0, 1.0},
		{"sent-rb-dlsr",		eUint, 0, 1.0}
};

/*********************************************************************************************
 * Description: Print the stats of a source                                                  *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void print_source_stats (GObject * source) {
  GstStructure *stats;
  gchar *str;

  g_return_if_fail (source != NULL);
  gint i = 0;
  const GValue *value = NULL;

  /* get the source stats */
  g_object_get (source, "stats", &stats, NULL);

  for (i = 0; i < RTCP_MAX_NUM; i++) {
	  value = gst_structure_get_value(stats, g_rtcp_parameter[i].key);
	  switch (g_rtcp_parameter[i].value_type) {
		  case eBollean: {
			  g_rtcp_parameter[i].value.value_bool = g_value_get_boolean(value);
			  g_print ("%s: %d\n", g_rtcp_parameter[i].key, g_rtcp_parameter[i].value.value_bool);
		  } break;
		  case eInt: {
			  g_rtcp_parameter[i].value.value_int = g_value_get_int(value);
			  g_print ("%s: %d\n", g_rtcp_parameter[i].key, g_rtcp_parameter[i].value.value_int);
		  } break;
		  case eUint: {
			  g_rtcp_parameter[i].value.value_uint = g_value_get_uint(value);
			  g_print ("%s: %u\n", g_rtcp_parameter[i].key, g_rtcp_parameter[i].value.value_uint);
		  } break;
		  case eInt64: {
			  g_rtcp_parameter[i].value.value_int64 = g_value_get_int64(value);
			  g_print ("%s: %ld\n", g_rtcp_parameter[i].key, g_rtcp_parameter[i].value.value_int64);
		  } break;
		  case eUint64: {
			  g_rtcp_parameter[i].value.value_uint64 = g_value_get_uint64(value);
			  g_print ("%s: %lu\n", g_rtcp_parameter[i].key, g_rtcp_parameter[i].value.value_uint64);
		  } break;
	  }
	 }

  /* simply dump the stats structure */
  str = gst_structure_to_string (stats);
  g_print ("source stats: %s\n", str);

  gst_structure_free (stats);
  g_free (str);
}


/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
#if 0
gboolean user_function (GstElement* element, GstPad *pad, void * pThis) {
  GstPad *sinkpad;
  GstPadLinkReturn lres;

  g_info ("Src pad name %s\n", GST_PAD_NAME (pad));
  sinkpad = gst_element_get_static_pad (pThis->rtcpsink, "sink");
  lres = gst_pad_link (pad, sinkpad);
  g_assert (lres == GST_PAD_LINK_OK);
  gst_object_unref (sinkpad);
}
#endif

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_new_ssrc_cb(GstElement * rtpbin, guint sessid, guint ssrc, RtspPipelineBundle *pThis) {
  GstPad *sinkpad, *srcpad;
  GstPadLinkReturn lres;

  g_print ("\nNew SSRC session %u, SSRC %u\n", sessid, ssrc);

  g_ssrc = ssrc;

  srcpad = gst_element_get_request_pad (rtpbin, "send_rtcp_src_0");
  if (gst_pad_is_linked (srcpad)) {
    g_print (" Src pad %s from already linked. Ignoring.\n", GST_PAD_NAME (srcpad));
  }
  // gst_element_foreach_src_pad(rtpbin, user_function, pThis);

}

/*********************************************************************************************
 * Description: will be called when rtpbin signals on-ssrc-active. It means that an RTCP     *
 * packet was received from another source.                                                  *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_ssrc_active_cb(GstElement * rtpbin, guint sessid, guint ssrc, RtspPipelineBundle *pThis) {
  GObject *session, *osrc;
  gdouble bandw;
  // GstStructure stats;
  
  /* get an RTCP srcpad for sending RTCP back to the sender */
  g_print ("\nGot RTCP from session %u, SSRC %u\n", sessid, g_ssrc);

  /* get the right session */
  /* Emit the "get-internal-session" signal on the bin with session id from callback*/
  g_signal_emit_by_name (rtpbin,  "get-internal-session", sessid, &session);

  /* get the remote source that sent us RTCP */
  g_signal_emit_by_name (session, "get-source-by-ssrc", g_ssrc, &osrc);
  print_source_stats(osrc);

  g_object_unref(osrc);
  g_object_unref(session);
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void new_rtp_session_manager_callback(GstElement *src, GstElement *manager, RtspPipelineBundle *pThis) {
  // @GST_RTSP_PROFILE_UNKNOWN: invalid profile
  // @GST_RTSP_PROFILE_AVP: the Audio/Visual profile (RFC 3551)
  // @GST_RTSP_PROFILE_SAVP: the secure Audio/Visual profile (RFC 3711)
  // @GST_RTSP_PROFILE_AVPF: the Audio/Visual profile with feedback (RFC 4585)
  // @GST_RTSP_PROFILE_SAVPF: the secure Audio/Visual profile with feedback (RFC 5124)

  // g_object_set(G_OBJECT(manager), "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);
  // g_print ("new_rtp_session_manager_callback\n");

  gchar *test = gst_element_get_name(manager);
  g_print("GstElement name %s\n", test);
  g_free(test);

  // Register a "on-ssrc-active" signal callback on the rtpbin element */
  // g_signal_connect (GST_ELEMENT(manager), "on-ssrc-active", G_CALLBACK (on_ssrc_active_cb), pThis);
  g_signal_connect (GST_ELEMENT(manager), "on-sender-ssrc-active", G_CALLBACK (on_ssrc_active_cb), pThis);
  g_signal_connect (GST_ELEMENT(manager), "on-new-ssrc", G_CALLBACK (on_new_ssrc_cb), pThis);

  // gst_element_link (GST_ELEMENT(manager), pThis->rtcpsink);

#if 0
  srcpad = gst_element_get_static_pad (manager, "send_rtcp_src_0");
  if (gst_pad_is_linked (srcpad)) {
    g_print (" Src pad from %s already linked. Ignoring.\n", GST_ELEMENT_NAME (manager));
  }
  // sinkpad = gst_element_get_static_pad (pThis->rtcpsink, "sink");
  // lres = gst_pad_link (srcpad, sinkpad);
  // g_assert (lres == GST_PAD_LINK_OK);
  // gst_object_unref (sinkpad); 
  gst_element_foreach_src_pad(manager, user_function, NULL);
#endif

  return;
}

/*********************************************************************************************
 * Description: pad added handler                                                            *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void pad_added_handler (GstElement *src, GstPad *new_pad, RtspPipelineBundle *pThis) {
  GstPad *sink_pad = gst_element_get_static_pad (pThis->depayloader, "sink");
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

  /* Check the new pad's name */
  if (!g_str_has_prefix (GST_PAD_NAME (new_pad), "recv_rtp_src_")) {
    g_print ("  It is not the right pad. Need recv_rtp_src_. Ignoring.\n");
    goto exit;
  }

  /* If our converter is already linked, we have nothing to do here */
  if (gst_pad_is_linked (sink_pad)) {
    g_print (" Sink pad from %s already linked. Ignoring.\n", GST_ELEMENT_NAME (src));
    goto exit;
  }

  /* Check the new pad's type */
  new_pad_caps = gst_pad_query_caps(new_pad, NULL); 
  // new_pad_caps = gst_pad_get_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);

  /* Attempt the link */
  ret = gst_pad_link (new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("  Type is '%s' but link failed.\n", new_pad_type);
  } else {
    g_print ("  Link succeeded (type '%s').\n", new_pad_type);
  }

  exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);

  /* Unreference the sink pad */
  gst_object_unref (sink_pad);
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
void clean_up(RtspPipelineBundle *p_appctx) {
  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (p_appctx->pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (p_appctx->pipeline));
}


/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data) {
  RtspPipelineBundle *p_appctx = (RtspPipelineBundle *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_info ("End of stream\n");
      clean_up(p_appctx);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      clean_up(p_appctx);
      break;
    }
    default:
      break;
  }
  return TRUE;
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
RtspPipelineBundle *start_pipeline(RTSPServerInfo *p_info, GMainLoop *loop)
{
  // GMainLoop *loop;
  GstBus *bus;
  gst_init(NULL, NULL);

  RtspPipelineBundle *p_appctx = g_malloc0(sizeof(RtspPipelineBundle));

  p_appctx->pipeline = gst_pipeline_new ("network-player");
  p_appctx->source = gst_element_factory_make ("rtspsrc","source");
  g_object_set (p_appctx->source, "do-rtcp", TRUE, NULL);

  p_appctx->depayloader = gst_element_factory_make ("rtph264depay", "depayloader");
  p_appctx->parser = gst_element_factory_make ("h264parse", "parser");
  p_appctx->decoder = gst_element_factory_make ("avdec_h264", "decoder");
  p_appctx->sink = gst_element_factory_make ("glimagesink", "sink");

  p_appctx->rtcpsink = gst_element_factory_make ("udpsink", "rtcpsink");
  g_object_set (p_appctx->rtcpsink, "port", 5007, "host", DEST_HOST, NULL);
  /* no need for synchronisation or preroll on the RTCP sink */
  g_object_set (p_appctx->rtcpsink, "async", FALSE, "sync", FALSE, NULL);

  if (!p_appctx->pipeline || !p_appctx->source || !p_appctx->depayloader || !p_appctx->parser || !p_appctx->decoder || !p_appctx->rtcpsink || !p_appctx->sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return NULL;
  }

  g_object_set(G_OBJECT(p_appctx->source), "location", p_info->location, NULL);
  g_object_set(G_OBJECT(p_appctx->source), "user-id",  p_info->user_id,  NULL);
  g_object_set(G_OBJECT(p_appctx->source), "user-pw",  p_info->user_pwd, NULL);

  bus = gst_pipeline_get_bus (GST_PIPELINE (p_appctx->pipeline));
  gst_bus_add_watch (bus, bus_call, p_appctx);
  gst_object_unref (bus);
	
  //then add all elements together
  gst_bin_add_many (GST_BIN (p_appctx->pipeline), p_appctx->source, p_appctx->depayloader, p_appctx->rtcpsink, \
                             p_appctx->parser, p_appctx->decoder, p_appctx->sink, NULL);

  //link everythink after source
  gst_element_link_many (p_appctx->depayloader, p_appctx->parser, p_appctx->decoder, p_appctx->sink, NULL);

  /*
   * Connect to the pad-added signal for the rtpbin.  This allows us to link
   * the dynamic RTP source pad to the depayloader when it is created.
   */
  g_signal_connect(p_appctx->source, "pad-added", G_CALLBACK (pad_added_handler), p_appctx);

  /* Register a "new-manager" signal callback on rtspsrc to get the rtpbin element */
  g_signal_connect(p_appctx->source, "new-manager", G_CALLBACK (new_rtp_session_manager_callback), p_appctx);


  /* Set the pipeline to "playing" state*/
  gst_element_set_state (p_appctx->pipeline, GST_STATE_PLAYING);

  /* Iterate */
  // g_print ("Running...\n");
  // g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  // g_print ("Returned, stopping playback\n");
  // gst_element_set_state (app.pipeline, GST_STATE_NULL);

  // g_print ("Deleting pipeline\n");
  // gst_object_unref (GST_OBJECT (app.pipeline));
  return p_appctx;
}
