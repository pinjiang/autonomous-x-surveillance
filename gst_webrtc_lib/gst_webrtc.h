#ifndef GST_WEBRTC_FUNC_H
#define GST_WEBRTC_FUNC_H

gboolean start_pipeline(InstanceCtx *ctx, const char *stunserver);

gboolean start_pipelines(InstanceCtx *ctx, const char *pipeline_str);

char * build_pipeline(ApplicationCtx * app_ctx_p, JsonArray *streams, const char *stunserver);

#endif
