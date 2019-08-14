#ifndef RTSP_LIB_H
#define RTSP_LIB_H

#include <gst/gst.h>

/*********************************************/
//RTCP 控制参数获取
#define RTCP_MAX_NUM		4
#define RTCP_LEY_LEN_MAX	64
typedef enum {
	eInvalid = -1, eBollean, eInt, eUint, eInt64, eUint64
}RtcpValueType;
typedef enum {
	eTypeInvalid = -1, eClockRate, eBitrate, eSentRbPacketsLost, eSentRbJitter
}RtcpType;
typedef union {
	gboolean	value_bool;
	gint 		value_int;
	guint 		value_uint;
	gint64		value_int64;
	guint64		value_uint64;
}RtcpValue;
typedef struct {
	RtcpType		id;
	gchar 			key[RTCP_LEY_LEN_MAX];
	RtcpValueType	value_type;	//数值类型
	gdouble			fk;			//数值系数
}RtcpParseInfo;
/*********************************************/
typedef struct {
   const char *location;
   const char *user_id;
   const char *user_pwd;
}RTSPServerInfo;

typedef struct myDataTag {
	//element 参数
	GstElement *pipeline;
	GstElement *source;
	GstElement *depayloader;
	GstElement *rtcpsink;
	GstElement *parser;
	GstElement *decoder;
	GstElement *sink;

	//运行参数
	guint		ssrc;
	RtcpValue	RtcpParseValue[RTCP_MAX_NUM];
} RtspPipelineBundle;

RtcpParseInfo g_rtcp_parameter[RTCP_MAX_NUM];

void gstream_init(void);
RtspPipelineBundle * start_pipeline(RTSPServerInfo *, GMainLoop *, const gchar*);
void clean_up(RtspPipelineBundle *p_appctx);

#endif
