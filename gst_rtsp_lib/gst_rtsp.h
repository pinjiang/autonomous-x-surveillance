#ifndef RTSP_LIB_H
#define RTSP_LIB_H

#include <gst/gst.h>

typedef struct {
   const char *location;
   const char *user_id;
   const char *user_pwd;
}RTSPServerInfo;

int start_pipeline(RTSPServerInfo *, GMainLoop *);

void clean_up(RtspPipelineBundle *p_appctx);

#endif
