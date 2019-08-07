#ifndef RTSP_LIB_H
#define RTSP_LIB_H

#include <gst/gst.h>

typedef struct {
   const char *location;
   const char *user_id;
   const char *user_pwd;
}RTSPServerInfo;

typedef struct myDataTag {
  GstElement *pipeline;
  GstElement *source;
  GstElement *depayloader;
  GstElement *rtcpsink;
  GstElement *parser;
  GstElement *decoder;
  GstElement *sink;
} RtspPipelineBundle;
/*********************************************/
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

RtcpParseInfo g_rtcp_parameter[RTCP_MAX_NUM];

RtspPipelineBundle * start_pipeline(RTSPServerInfo *, GMainLoop *);
void clean_up(RtspPipelineBundle *p_appctx);

void clean_up();

#endif
