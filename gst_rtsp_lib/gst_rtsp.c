#include <glib.h>
#include <gst/gst.h>

#include "gst_rtsp.h"

/* the destination machine to send RTCP to. This is the address of the sender and
 * is used to send back the RTCP reports of this receiver. If the data is sent
 * from another machine, change this address. */
#define DEST_HOST "127.0.0.1"

//外部使用参数
RtcpParseInfo g_rtcp_parameter[RTCP_MAX_NUM] = {
		{eSSRC, 			"ssrc",			 		eUint, 1.0, FALSE},
		{eRECEIVED_BYE,	"received-bye",			eBollean, 1.0, FALSE},
		{eClockRate,	 	"clock-rate", 			eInt, 1.0, TRUE},
		{eOCTETS_RECVIVED,"octets-received", 		eUint64, 1.0, FALSE},
		{ePACKETS_RECEIVED,"packets-received", 	eUint64, 1.0, FALSE},
		{eBitrate,		"bitrate", 				eUint64, 1.0, TRUE},
		{ePACKETS_LOST,	"packets-lost", 		eInt, 1.0, FALSE},
		{eJITTER,			"jitter", 				eUint, 1.0, FALSE},
		{eSENT_PLI_COUNT,	"sent-pli-count", 		eUint, 1.0, FALSE},
		{eRECV_PLI_COUNT,	"recv-pli-count", 		eUint, 1.0, FALSE},
		{eSENT_FIR_COUNT,	"sent-fir-count", 		eUint, 1.0, FALSE},
		{eRECV_FIR_COUNT,	"recv-fir-count", 		eUint, 1.0, FALSE},

		{eSR_NTPTIME,		"sr-ntptime", 			eUint, 1.0, FALSE},
		{eSR_RTPTIME,		"sr-rtptime",			eUint, 1.0, FALSE},
		{eSR_OCTET_COUNT,	"sr-octet-count",		eUint, 1.0, FALSE},
		{eSR_PACKET_COUNT,"sr-packet-count",		eUint, 1.0, FALSE},

		{eSENT_RB_FRACTIONLOST,	"sent-rb-fractionlost",	eUint, 1.0, FALSE},
		{eSentRbPacketsLost, 		"sent-rb-packetslost",	eInt, 1.0, TRUE},
		{eSENT_RB_EXTHIGHESTSEQ,	"sent-rb-exthighestseq",eUint, 1.0, FALSE},
		{eSentRbJitter, 	 		"sent-rb-jitter",		eUint, 1000.0, TRUE},
		{eSENT_RB_LSR,			"sent-rb-lsr",			eUint, 1.0, FALSE},
		{eSENT_RB_DLSR,			"sent-rb-dlsr",			eUint, 1.0, FALSE}


//		[eSSRC] 				= {eSSRC, 			"ssrc",			 		eUint, 1.0, FALSE},
//		[eRECEIVED_BYE] 		= {eRECEIVED_BYE,	"received-bye",			eBollean, 1.0, FALSE},
//		[eClockRate] 			= {eClockRate,	 	"clock-rate", 			eInt, 1.0, TRUE},
//		[eOCTETS_RECVIVED] 		= {eOCTETS_RECVIVED,"octets-received", 		eUint64, 1.0, FALSE},
//		[ePACKETS_RECEIVED] 	= {ePACKETS_RECEIVED,"packets-received", 	eUint64, 1.0, FALSE},
//		[eBitrate] 				= {eBitrate,		"bitrate", 				eUint64, 1.0, TRUE},
//		[ePACKETS_LOST] 		= {ePACKETS_LOST,	"packets-lost", 		eInt, 1.0, FALSE},
//		[eJITTER] 				= {eJITTER,			"jitter", 				eUint, 1.0, FALSE},
//		[eSENT_PLI_COUNT]		= {eSENT_PLI_COUNT,	"sent-pli-count", 		eUint, 1.0, FALSE},
//		[eRECV_PLI_COUNT] 		= {eRECV_PLI_COUNT,	"recv-pli-count", 		eUint, 1.0, FALSE},
//		[eSENT_FIR_COUNT] 		= {eSENT_FIR_COUNT,	"sent-fir-count", 		eUint, 1.0, FALSE},
//		[eRECV_FIR_COUNT] 		= {eRECV_FIR_COUNT,	"recv-fir-count", 		eUint, 1.0, FALSE},
//
//		[eSR_NTPTIME] 			= {eSR_NTPTIME,		"sr-ntptime", 			eUint, 1.0, FALSE},
//		[eSR_RTPTIME] 			= {eSR_RTPTIME,		"sr-rtptime",			eUint, 1.0, FALSE},
//		[eSR_OCTET_COUNT] 		= {eSR_OCTET_COUNT,	"sr-octet-count",		eUint, 1.0, FALSE},
//		[eSR_PACKET_COUNT] 		= {eSR_PACKET_COUNT,"sr-packet-count",		eUint, 1.0, FALSE},
//
//		[eSENT_RB_FRACTIONLOST] = {eSENT_RB_FRACTIONLOST,	"sent-rb-fractionlost",	eUint, 1.0, FALSE},
//		[eSentRbPacketsLost] 	= {eSentRbPacketsLost, 		"sent-rb-packetslost",	eInt, 1.0, TRUE},
//		[eSENT_RB_EXTHIGHESTSEQ]= {eSENT_RB_EXTHIGHESTSEQ,	"sent-rb-exthighestseq",eUint, 1.0, FALSE},
//		[eSentRbJitter] 		= {eSentRbJitter, 	 		"sent-rb-jitter",		eUint, 1000.0, TRUE},
//		[eSENT_RB_LSR] 			= {eSENT_RB_LSR,			"sent-rb-lsr",			eUint, 1.0, FALSE},
//		[eSENT_RB_DLSR] 		= {eSENT_RB_DLSR,			"sent-rb-dlsr",			eUint, 1.0, FALSE}
};

/*********************************************************************************************
 * Description: 解析获取数据帧的状态                                                 			 *
 *                                                                                           *
 * Input :  GObject * source：
 * 			RtspPipelineBundle *this：RTSP的结构体                                             *
 * Return: 	无                                                                                *
 *********************************************************************************************/
static void print_source_stats (GObject * source, RtspPipelineBundle *this) {
	GstStructure *stats;
	gchar *str;
	gint i = 0;
	const GValue *value = NULL;

	/* get the source stats */
	g_object_get (source, "stats", &stats, NULL);

	for (i = 0; i < RTCP_MAX_NUM; i++) {
	  value = gst_structure_get_value(stats, g_rtcp_parameter[i].key);
	  switch (g_rtcp_parameter[i].value_type) {
		  case eBollean: {
			  this->RtcpParseValue[i].value_bool = g_value_get_boolean(value);
		  } break;
		  case eInt: {
			  this->RtcpParseValue[i].value_int = \
					  g_value_get_int(value) * g_rtcp_parameter[i].fk;
		  } break;
		  case eUint: {
			  this->RtcpParseValue[i].value_uint = \
					  g_value_get_uint(value) * g_rtcp_parameter[i].fk;
		  } break;
		  case eInt64: {
			  this->RtcpParseValue[i].value_int64 = \
					  g_value_get_int64(value) * g_rtcp_parameter[i].fk;
		  } break;
		  case eUint64: {
			  this->RtcpParseValue[i].value_uint64 = \
					  g_value_get_uint64(value) * g_rtcp_parameter[i].fk;
		  } break;
	  }
	}

	/* simply dump the stats structure */
	str = gst_structure_to_string (stats);
	g_print ("source stats: %s\n", str);
	g_free (str);

	gst_structure_free (stats);

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
 * Description: 获取新的SSRC的回调                                                             *
 *                                                                                           *
 * Input :	GstElement * rtpbin：箱柜
 * 			guint sessid：id号
 * 			guint ssrc：ssrc 序号
 * 			RtspPipelineBundle *pThis：RTSP的结构体                                            *
 * Return:  无                                                                                *
 *********************************************************************************************/
static void on_new_ssrc_cb(GstElement * rtpbin, guint sessid, guint ssrc, RtspPipelineBundle *pThis) {
	GstPad *sinkpad, *srcpad;
	GstPadLinkReturn lres;

	g_print ("\nNew SSRC session %u, SSRC %u\n", sessid, ssrc);

	pThis->ssrc = ssrc;

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
 * Input :	GstElement * rtpbin：箱柜
 * 			guint sessid：id号
 * 			guint ssrc：ssrc 序号
 * 			RtspPipelineBundle *pThis：RTSP的结构体                                            *
 * Return:  无                                                                                 *
 *********************************************************************************************/
static void on_ssrc_active_cb(GstElement * rtpbin, guint sessid, guint ssrc, RtspPipelineBundle *pThis) {
	GObject *session = NULL, *osrc = NULL;
	gdouble bandw = 0.0;

	/* get an RTCP srcpad for sending RTCP back to the sender */
	g_print ("\nGot RTCP from session %u, SSRC %u\n", sessid, pThis->ssrc);

	/* get the right session */
	/* Emit the "get-internal-session" signal on the bin with session id from callback*/
	g_signal_emit_by_name (rtpbin,  "get-internal-session", sessid, &session);
	/* get the remote source that sent us RTCP */
	g_signal_emit_by_name (session, "get-source-by-ssrc", pThis->ssrc, &osrc);
	if (NULL != osrc) {
		print_source_stats(osrc, pThis);
	}


	if (NULL != osrc) g_object_unref(osrc);
	if (NULL != session) g_object_unref(session);
}

/*********************************************************************************************
 * Description: 新的rtp会话消息回调函数                                                         *
 *                                                                                           *
 * Input : 	GstElement *src：回调对象
 * 			GstElement *manager：元件
 * 			RtspPipelineBundle *pThis：RTSP的结构体                                            *
 * Return:  无                                                                                *
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
 * Description: 新的pad回调函数                                                            	 *
 *                                                                                           *
 * Input : 	GstElement *src：产生回调的元件
 * 			GstPad *new_pad：新的pad
 * 			RtspPipelineBundle *pThis：RTSP的结构体                                            *
 * Return:  无                                                                                 *
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
	if (new_pad_caps != NULL) {
		gst_caps_unref (new_pad_caps);
	}
	/* Unreference the sink pad */
	gst_object_unref (sink_pad);
}

/*********************************************************************************************
 * Description: pipeline监视异常回调函数                                                       *
 *                                                                                           *
 * Input : 	GstBus *bus：触发bus的结构
 * 			GstMessage *msg：消息
 * 			gpointer data：回调传入的参数                                                                                  *
 * Return:  TRUE：成功                                                                                *
 *********************************************************************************************/
static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data) {
  RtspPipelineBundle *p_appctx = (RtspPipelineBundle *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
		g_info("End of stream\n");
		break;

    case GST_MESSAGE_ERROR: {
		gchar  *debug;
		GError *error;

		gst_message_parse_error (msg, &error, &debug);
		g_free (debug);

		g_warning("Error: %s\n", error->message);
		g_error_free (error);

		break;
    }
    default:
      break;
  }
  return TRUE;
}

/*********************************************************************************************
 * Description: 开启视频管道                                                                   *
 *                                                                                           *
 * Input :  RTSPServerInfo *p_info：链接的RTP服务器信息
 * 			GMainLoop *loop：主循环
 * 			const gchar *pipe_name：管道名称                                                   *
 * Return:  NULL：失败	非NULL：打开成功                                                       *
 *********************************************************************************************/
RtspPipelineBundle *start_pipeline(RTSPServerInfo *p_info, GMainLoop *loop, const gchar *pipe_name)
{
	if (NULL == p_info || NULL == loop || NULL == pipe_name) {
		return NULL;
	}

	GstBus *bus = NULL;
	guint bus_id = 0;
	GstStateChangeReturn ret = 0;
	/* 1.申请空间 */
	RtspPipelineBundle *appctx = g_malloc0(sizeof(RtspPipelineBundle));
	if (NULL == appctx) {
		return NULL;
	}

	/* 2.创建元件 */
	appctx->pipeline = gst_pipeline_new (pipe_name);
	appctx->source = gst_element_factory_make ("rtspsrc","source");
	if (NULL != appctx->source) {
		g_object_set (appctx->source, "do-rtcp", TRUE, NULL);
	}

	appctx->depayloader = gst_element_factory_make ("rtph264depay", "depayloader");
	appctx->parser = gst_element_factory_make ("h264parse", "parser");
	appctx->decoder = gst_element_factory_make ("avdec_h264", "decoder");
	appctx->sink = gst_element_factory_make ("glimagesink", "sink");

	appctx->rtcpsink = gst_element_factory_make ("udpsink", "rtcpsink");
	if (NULL != appctx->rtcpsink) {
		g_object_set (appctx->rtcpsink, "port", 5007, "host", DEST_HOST, NULL);
		/* no need for synchronisation or preroll on the RTCP sink */
		g_object_set (appctx->rtcpsink, "async", FALSE, "sync", FALSE, NULL);
	}
	if (!appctx->pipeline || !appctx->source || !appctx->depayloader \
		|| !appctx->parser || !appctx->decoder || !appctx->rtcpsink \
		|| !appctx->sink) {
		g_warning ("One element could not be created. Exiting.\n");
		goto ERR;
	}

	g_object_set(G_OBJECT(appctx->source), "location", p_info->location, NULL);
	g_object_set(G_OBJECT(appctx->source), "user-id",  p_info->user_id,  NULL);
	g_object_set(G_OBJECT(appctx->source), "user-pw",  p_info->user_pwd, NULL);

	bus = gst_pipeline_get_bus (GST_PIPELINE (appctx->pipeline));
	if (NULL == bus) {
		g_warning ("Get bus of pipeline err. Exiting.\n");
		goto ERR;
	}
	bus_id = gst_bus_add_watch (bus, bus_call, appctx);
	if (0 == bus_id) {
		g_warning ("Bus add watch err. Exiting.\n");
		gst_object_unref (bus);
		goto ERR;
	}
	gst_object_unref (bus);

	//then add all elements together
	gst_bin_add_many (GST_BIN (appctx->pipeline), appctx->source, appctx->depayloader, appctx->rtcpsink, \
							 appctx->parser, appctx->decoder, appctx->sink, NULL);
	//link everythink after source
	if (FALSE == gst_element_link_many (appctx->depayloader, appctx->parser, appctx->decoder, \
			appctx->sink, NULL)) {
		g_warning ("Pipeline link many elements err. Exiting.\n");
		goto ERR_ADD;
	}

	/*
	* Connect to the pad-added signal for the rtpbin.  This allows us to link
	* the dynamic RTP source pad to the depayloader when it is created.
	*/
	g_signal_connect(appctx->source, "pad-added", G_CALLBACK (pad_added_handler), appctx);

	/* Register a "new-manager" signal callback on rtspsrc to get the rtpbin element */
	g_signal_connect(appctx->source, "new-manager", G_CALLBACK (new_rtp_session_manager_callback), appctx);

	/* Set the pipeline to "playing" state*/
	ret = gst_element_set_state (appctx->pipeline, GST_STATE_PLAYING);
	if (GST_STATE_CHANGE_FAILURE == ret) {
		g_warning ("Pipeline state change err(playing). Exiting.\n");
		goto ERR_ADD;
	}
	
	return appctx;

ERR:
	if (NULL != appctx) {
		if (NULL != appctx->pipeline) 	gst_object_unref (GST_OBJECT (appctx->pipeline));
		if (NULL != appctx->source) 	gst_object_unref (GST_OBJECT (appctx->source));
		if (NULL != appctx->depayloader) gst_object_unref (GST_OBJECT (appctx->depayloader));
		if (NULL != appctx->parser) 	gst_object_unref (GST_OBJECT (appctx->parser));
		if (NULL != appctx->decoder) 	gst_object_unref (GST_OBJECT (appctx->decoder));
		if (NULL != appctx->rtcpsink) 	gst_object_unref (GST_OBJECT (appctx->rtcpsink));
		if (NULL != appctx->sink) 		gst_object_unref (GST_OBJECT (appctx->sink));
		g_free(appctx);
		appctx = NULL;
	}
	return NULL;
ERR_ADD:
	if (NULL != appctx) {
		if (NULL != appctx->pipeline) 	gst_object_unref (GST_OBJECT (appctx->pipeline));
		g_free(appctx);
		appctx = NULL;
	}
	return NULL;
}


/*********************************************************************************************
 * Description: 清理管道数据                                                                   *
 *                                                                                           *
 * Input : RtspPipelineBundle *p_appctx：需要清理的RTSP的结构体                                 *
 * Return: 无                                                                                 *
 *********************************************************************************************/
void clean_up(RtspPipelineBundle *p_appctx) {
	/* Out of the main loop, clean up nicely */
	if (NULL != p_appctx) {
		if (NULL != p_appctx->pipeline) {
			g_info ("Returned, stopping playback\n");
			gst_element_set_state (p_appctx->pipeline, GST_STATE_NULL);
			g_info ("Deleting pipeline\n");
			gst_object_unref (GST_OBJECT (p_appctx->pipeline));
			p_appctx->pipeline = NULL;
		}
		g_free(p_appctx);
		p_appctx = NULL;
		g_info ("Free pipeline\n");
	}
}

/*********************************************************************************************
 * Description: gstream 初始化                                                                *
 *                                                                                           *
 * Input : 无                                                                                  *
 * Return: 无                                                                                  *
 *********************************************************************************************/
void gstream_init(void)
{
	gst_init(NULL, NULL);
}

