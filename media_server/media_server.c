/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003, Ximian, Inc.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <glib/gstdio.h>

#include "media_server.h"
#include "utils.h"
#include "gst_rtsp.h"

enum {
	eMEDIA_SERVER_SUCCESS	= 0,
	eMEDIA_SERVER_FAILE		= -1,
	eMEDIA_SERVER_INVALID	= -2,

	eMSG_GET_ERR 			= -200,
	eMSG_JSON_PARSE_ERR 	= -201,
	eMSG_JSON_LOAD_ERR 		= -202,
	eMSG_JSON_GET_ROOT_ERR 	= -203,
	eMSG_JSON_GET_TYPE_ERR 	= -204,
	eMSG_JSON_TYPE_VALUE_ERR= -205,
	eMSG_JSON_TYPE_NOT_FIND	= -206,

	eMSG_JSON_PLAY_FIND_MEMBER_ERR 	= -220,
	eMSG_JSON_PLAY_GET_MEMBER_ERR 	= -221,
	eMSG_JSON_PLAY_KEY_MALLOC_ERR 	= -222,
	eMSG_JSON_PLAY_VIDEO_MALLOC_ERR = -223,
	eMSG_JSON_PLAY_NAME_MALLOC_ERR 	= -224,
	eMSG_JSON_PLAY_URL_MALLOC_ERR 	= -225,
	eMSG_JSON_PLAY_HASH_INSTALL_ERR = -226,
	eMSG_JSON_PLAY_START_VIDEO_ERR 	= -227,
	eMSG_JSON_PLAY_KEY_ALREADY_EXIST= -228,

	eMSG_JSON_STOP_FIND_MEMBER_ERR 	= -230,
	eMSG_JSON_STOP_GET_MEMBER_ERR 	= -231,
	eMSG_JSON_STOP_HASH_REMOVE_ERR 	= -232,
	eMSG_JSON_STOP_HASH_FIND_ERR 	= -233,
	eMSG_JSON_STOP_HASH_VLAUE_ERR 	= -234,

	eMSG_JSON_REPLY_BUILD_ERR 		= -240,
	eMSG_JSON_GENERATOR_NEW_ERR 	= -241,
	eMSG_JSON_BUILD_GET_ROOT_ERR 	= -242,
	eMSG_JSON_GENERATOR_TO_DATA_ERR = -243,
};

/*******************************************************************************************************/
//请求消息为字符串类型
#define MSG_TEX_TYPE_MAX			2
#define MSG_TEXT_STRING_LEN_MAX 	64
typedef gint (*MsgTexSubParseRequst)(gpointer msg_data, gchar **reply, gpointer user_data);
typedef struct {
	gchar 					type[MSG_TEXT_STRING_LEN_MAX];
	MsgTexSubParseRequst	parse_request;
}MsgTexType;

//视频存储信息
typedef enum {
	eVIDEO_STATUS_UNUSED = -1,
	eVIDEO_STATUS_INIT = 0,
	eVIDEO_STATUS_PLAY,
	eVIDEO_STATUS_STOP
}VideoStatus;
typedef struct {
	gchar 		*name;
	gchar 		*url;
	VideoStatus status;
	RtspPipelineBundle 	*pipe;
}VideoInfo;

void stop_server(SoupServer* server);

/*******************************************************************************************************/
//内部使用函数
static void websocket_callback (SoupServer *server, SoupWebsocketConnection *connection,
								const char *path, SoupClientContext *client, gpointer user_data);
static gboolean send_rtcp_callback (gpointer user_data);
///解析websocket的数据
static void on_message(SoupWebsocketConnection *conn, gint type, GBytes *message, gpointer data);
////websocket的字符串信息处理函数
static gint on_text_message (SoupWebsocketConnection *ws, SoupWebsocketDataType type, \
							GBytes *message, gpointer user_data);
static gint parse_text_msg_on_json(const gchar *msg_ptr, gsize length, gchar **msg_reply, gpointer user_data);
static gint msg_requst_type_paly(gpointer msg_data, gchar **reply, gpointer user_data);
static gint msg_requst_type_stop(gpointer msg_data, gchar **reply, gpointer user_data);
static gint reply_text_msg_on_json(gchar **reply, const gchar *member, const gchar *value);
////websocket的二进制信息处理函数
static gint on_binary_message (SoupWebsocketConnection *ws, SoupWebsocketDataType type, \
		   	   	   	   	   	   GBytes *message,  gpointer user_data);

static gboolean hash_table_sub_remove_call (gpointer key, gpointer value, gpointer user_data);
static void hash_remove_key_call (gpointer data);
static void hash_remove_value_call (gpointer data);
/*******************************************************************************************************/
//全局变量
static gboolean is_wss = FALSE; /* Temporary variable */
static const MsgTexType g_msg_text_type[MSG_TEX_TYPE_MAX] = {
		{"play", msg_requst_type_paly},
		{"stop", msg_requst_type_stop},
};


static GHashTable *g_hash_video_info = NULL;


/************************************ WEB SOCKET 方式 *********************************************************/
/*********************************************************************************************
 * Description: 开启websocket服务                                                             *
 *                                                                                           *
 * Input : const int port：监听端口
 * 		   const char *tls_cert_file：证书文件												 *
 * 		   const char *tls_key_file：密钥文件													 *
 * 		   AppContext *app：应用参数                                                           *
 * Return: 开启的服务                                                                          *
 *********************************************************************************************/
SoupServer* start_server(const int port, const char *tls_cert_file, const char *tls_key_file, AppContext *app) {

	if (port <= 0 || NULL == app) {
		return NULL;
	}
	GSList *uris = NULL, *u = NULL;
	GTlsCertificate *cert = NULL;
	gchar *str = NULL;
	GError *error = NULL;
	SoupServer *server = NULL;

	/* 1.创建视频信息存储HashMap */
	g_hash_video_info = g_hash_table_new_full(g_str_hash, g_str_equal, \
			hash_remove_key_call, hash_remove_value_call);
	if (NULL == g_hash_video_info) {
		g_error ("Creat video hashmap err\n");
		return NULL;
	}

	/* 2.创建监听服务 */
	if ( tls_cert_file && tls_key_file ) {
		cert = g_tls_certificate_new_from_files (tls_cert_file, tls_key_file, &error);
		if (error) {
			  g_error ("Unable to create server: %s\n", error->message);
			  stop_server(NULL);
			  return NULL;
		}
		server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "simple-httpd ",
								 SOUP_SERVER_TLS_CERTIFICATE, cert, NULL);
		g_object_unref (cert);
		soup_server_listen_local (server, port, SOUP_SERVER_LISTEN_HTTPS, &error);
	} else {
		server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "simple-httpd ", NULL);
		soup_server_listen_local (server, port, 0, &error);
	}

	/* 3.增加WebSocket处理函数 */
	soup_server_add_websocket_handler(server, NULL, NULL, NULL,
								   websocket_callback,
								   app, NULL);
	//创建定时任务去发送RTCP的数据
	GSource *source = g_timeout_source_new (1500);
	guint sourceID = g_source_attach (source, NULL);
	glib_log_info("source_get_ID:%d\n", sourceID);
	g_source_set_callback (source, send_rtcp_callback, app, NULL);

	uris = soup_server_get_uris (server);
	for (u = uris; u; u = u->next) {
		str = soup_uri_to_string (u->data, FALSE);
		g_info ("Listening on %s", str);
		g_free (str);
		soup_uri_free (u->data);
	}
	g_slist_free (uris);
	g_info ("Waiting for requests...");

	return server;
}

/*********************************************************************************************
 * Description: 停止websocket服务， 释放资源                                                    *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
void stop_server(SoupServer* server)
{
	//停止服务
	if (NULL != server) {

	}
	//释放hashmap的值
	if (NULL != g_hash_video_info) {
		g_hash_table_foreach_remove(g_hash_video_info, hash_table_sub_remove_call, NULL);
		g_hash_table_destroy (g_hash_video_info);
	}
}

/*********************************************************************************************
 * Description: 整理视频状态推送结构                                                 *
 * {
	"type": "report",
    "name": "front",
	"stats":
	}
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gboolean send_rtcp_callback (gpointer user_data)
{
	glib_log_info("send_rtcp_callback");
	if (NULL == user_data) {
		glib_log_info("send_rtcp_callback data is NULL");
		return TRUE;
	}
	gint i = 0;
	gint ret = eMEDIA_SERVER_SUCCESS;
	JsonGenerator *gen = NULL;
	JsonNode 	  *root= NULL;
	AppContext 	*app = user_data;

	if (NULL == app->server) {
		glib_log_info("send_rtcp_callback app->server is NULL");
		return TRUE;
	}
	/* 1.创建JSON */
	JsonBuilder *builder = json_builder_new ();
	if (NULL == builder) {
		glib_log_warning ("Build  JsonBuilder err");
		return TRUE;
	}
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "type");
	json_builder_add_string_value(builder, "report");
	json_builder_set_member_name(builder, "name");
	json_builder_add_string_value(builder, "front");

	for (i = 0; i < RTCP_MAX_NUM; i++) {
		json_builder_set_member_name(builder, g_rtcp_parameter[i].key);
		switch (g_rtcp_parameter[i].value_type) {
		 case eBollean: {
			 json_builder_add_boolean_value(builder, g_rtcp_parameter[i].value.value_bool);
		  } break;
		  case eInt: {
			  json_builder_add_int_value(builder, g_rtcp_parameter[i].value.value_int);
		  } break;
		  case eUint: {
			  json_builder_add_int_value(builder, g_rtcp_parameter[i].value.value_uint);
		  } break;
		  case eInt64: {
			  json_builder_add_int_value(builder, g_rtcp_parameter[i].value.value_int64);
		  } break;
		  case eUint64: {
			  json_builder_add_int_value(builder, g_rtcp_parameter[i].value.value_uint64);
		  } break;
		}
	}
	json_builder_end_object(builder);

	/* 2.将JSON转化为字符串 */
	gen = json_generator_new();
	if (NULL == gen) {
		g_object_unref (builder);
		return TRUE;
	}
	root = json_builder_get_root(builder);
	if (NULL == root) {
		g_object_unref (gen);
		g_object_unref (builder);
		return TRUE;
	}
	json_generator_set_root(gen, root);
	gchar *reply = json_generator_to_data(gen, NULL);
	if (NULL != reply) {
		soup_websocket_connection_send_text(app->server, reply);
		g_free(reply);
	}

	/* 3.清理资源 */
	json_node_free (root);
	g_object_unref (gen);
	g_object_unref (builder);

	return TRUE;
}
/*********************************************************************************************
 * Description: 释放HashMap资源，回调函数                                                       *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gboolean hash_table_sub_remove_call (gpointer key, gpointer value, gpointer user_data)
{
	g_assert(NULL != key && NULL != value);
	glib_log_info("Hash remove key:%s", (gchar*)key);
	if (NULL != key) {
		g_free(key);
	}
	VideoInfo *video = value;
	if (NULL != video) {
		if (NULL != video->url) 	g_free(video->url);
		if (NULL != video->name) 	g_free(video->name);
		if (NULL != video->pipe)	clean_up(video->pipe);
		g_free(video);
	}
	return TRUE;
}

static void hash_remove_key_call (gpointer data)
{
	gchar *key = (gchar *)data;
	if (NULL != key) {
		glib_log_info("Remove key:%s", (gchar*)key);
		g_free(key);
	}
}

static void hash_remove_value_call (gpointer data)
{
	VideoInfo *video = (VideoInfo *)data;
	if (NULL != video) {
		glib_log_info("Remove value:%s", video->name);
		if (NULL != video->url) 	g_free(video->url);
		if (NULL != video->name) 	g_free(video->name);
		if (NULL != video->pipe)	clean_up(video->pipe);
		g_free(video);
	}
}
/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_close(SoupWebsocketConnection *conn, gpointer data) {
  soup_websocket_connection_close(conn, SOUP_WEBSOCKET_CLOSE_NORMAL, NULL);
  g_info("WebSocket connection closed\n");
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_connection(SoupSession *session, GAsyncResult *res, gpointer data) {
  SoupWebsocketConnection *conn;
  GError *error = NULL;

  conn = soup_session_websocket_connect_finish(session, res, &error);
  if (error) {
    g_print("Error: %s\n", error->message);
    g_error_free(error);
    return;
  }
  g_signal_connect(conn, "message", G_CALLBACK(on_message), NULL);
  g_signal_connect(conn, "closed",  G_CALLBACK(on_close),   NULL);

  soup_websocket_connection_send_text(conn, (is_wss) ? "Hello Secure Websocket !" : "Hello Websocket !");
}

/*********************************************************************************************
 * Description: websocket 回调函数                                                            *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void websocket_callback (SoupServer *server, SoupWebsocketConnection *connection,
     const char *path, SoupClientContext *client, gpointer user_data ) {

	GBytes *received = NULL;
	g_info("websocket connected");

	AppContext *ctx = user_data;
	ctx->server = g_object_ref (connection);
	g_signal_connect (ctx->server, "message", G_CALLBACK (on_message), &received);

	return;
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void on_message(SoupWebsocketConnection *conn, gint type, GBytes *message, gpointer data) {

	g_assert (conn != NULL && message != NULL && data != NULL);

	gint ret = eMEDIA_SERVER_SUCCESS;
	AppContext *app = data;
	RTSPServerInfo info;

	/* 1.根据消息类型处理 */
	switch (type) {
		//文本消息
		case SOUP_WEBSOCKET_DATA_TEXT: {
			ret = on_text_message(conn, type, message, data);
		} break;
		//二进制消息
		case SOUP_WEBSOCKET_DATA_BINARY: {
			glib_log_info("Received binary data (not shown)");
		} break;
		//其他消息
		default: {
			glib_log_info("Invalid data type: %d", type);
		} break;
	}

	/* 2.处理结果 */
	if (eMEDIA_SERVER_SUCCESS != ret) {
		glib_log_warning("Msg type:%d Parse ret:%d", type, ret);
	}
	glib_log_info("Msg type:%d Parse ret:%d", type, ret);
}

/*********************************************************************************************
 * Description: 处理字符类型信息                                                               *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gint on_text_message (SoupWebsocketConnection *ws,
                 SoupWebsocketDataType type,
                 GBytes *message,
                 gpointer user_data) {
	g_assert (ws != NULL && message != NULL && user_data != NULL);
	g_assert_cmpint (type, ==, SOUP_WEBSOCKET_DATA_TEXT);

	gint 		ret		= 0;
	gsize 		length 	= 0;
	const gchar *msg_ptr = NULL;
	gchar *msg_reply 	= NULL;

	//1.获取字符
	msg_ptr = g_bytes_get_data(message, &length);
	if (NULL == msg_ptr || 0 == length) {
		glib_log_warning("Get text err");
		return eMSG_GET_ERR;
	}
	glib_log_info("Recv[%ld]:%s", length, msg_ptr);

	//2.以JSON格式解析数据并回复
	ret = parse_text_msg_on_json(msg_ptr, length, &msg_reply, user_data);
	if (NULL != msg_reply) {
		//回复浏览器处理结果
		soup_websocket_connection_send_text(ws, msg_reply);
		g_free(msg_reply);
		msg_reply = NULL;
	}

	return ret;
}

/*********************************************************************************************
 * Description: 处理二进制信息                                                                 *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gint on_binary_message (SoupWebsocketConnection *ws,
		   SoupWebsocketDataType type,
		   GBytes *message,
		   gpointer user_data) {
	GBytes **receive = user_data;

	g_assert_cmpint (type, ==, SOUP_WEBSOCKET_DATA_BINARY);
	g_assert (*receive == NULL);
	g_assert (message != NULL);
}


/*********************************************************************************************
 * Description:                                                               *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gint parse_text_msg_on_json(const gchar *msg_ptr, gsize length, gchar **msg_reply, gpointer user_data)
{
	g_assert (msg_ptr != NULL && 0 != length);

	gint 		ret		= eMEDIA_SERVER_SUCCESS;
	GError     	*error  = NULL;
	JsonParser 	*parser = NULL;
	JsonNode 	*root	= NULL;
	JsonObject	*object = NULL;
	const gchar *str_type	 = NULL;

	/* 1. 解析为JSON格式 */
	parser = json_parser_new();
	if (NULL == parser) {
		return eMSG_JSON_PARSE_ERR;
	}
	if (TRUE != json_parser_load_from_data (parser, msg_ptr, length, &error)) {
		glib_log_warning("Msg json parse err:%s", error->message);
		ret = eMSG_JSON_LOAD_ERR;
		goto ERR;
	}
	/* 2.获取节点 "type" */
	root = json_parser_get_root (parser);
	if (NULL == root || !JSON_NODE_HOLDS_OBJECT(root)) {
		glib_log_warning("Msg json parse err:%s", error->message);
		ret = eMSG_JSON_GET_ROOT_ERR;
		goto ERR;
	}
	object = json_node_get_object (root);
	if (!json_object_has_member (object, "type")) {
		glib_log_warning ("received message without 'type'");
		ret = eMSG_JSON_GET_TYPE_ERR;
		goto ERR;
	}
	str_type = json_object_get_string_member (object, "type");
	if (NULL == str_type) {
		glib_log_warning ("Value of 'type' is NULL");
		ret = eMSG_JSON_TYPE_VALUE_ERR;
		goto ERR;
	}
	glib_log_debug("type:%s", str_type);
	/* 3.根据类型调用相应的处理函数 */
	gint i = 0;
	for (i = 0; i < MSG_TEX_TYPE_MAX; i++) {
		if (0 == g_strcmp0 (str_type, g_msg_text_type[i].type)) {
			if (NULL != g_msg_text_type[i].parse_request) {
				ret = g_msg_text_type[i].parse_request(object, msg_reply, user_data);
			}
			break;
		}
	}
	if (i >= MSG_TEX_TYPE_MAX) {
		glib_log_warning ("Type of %s not find", str_type);
		ret = eMSG_JSON_TYPE_NOT_FIND;
		goto ERR;
	}

ERR:
	if (NULL != error) 	g_error_free (error);
	if (NULL != parser)	g_object_unref (parser);
	return ret;
}

/*********************************************************************************************
 * Description: play命令解析                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gint msg_requst_type_paly(gpointer msg_data, gchar **reply, gpointer user_data)
{
	g_assert (NULL != msg_data && NULL != reply && NULL != user_data);

	gint		ret		= eMEDIA_SERVER_SUCCESS;
	const gchar	*name 	= NULL,	*url	= NULL;
	JsonObject 	*object 	= msg_data;
	RTSPServerInfo info = {0};
	AppContext 	*app = user_data;
	gchar		*key = NULL;
	VideoInfo 	*video = NULL;

	//1.查询需要的字段并获取
	if (!json_object_has_member (object, "name") \
		|| !json_object_has_member (object, "url")) {

		glib_log_warning ("Received message without 'name' || 'url'");
		return eMSG_JSON_PLAY_FIND_MEMBER_ERR;
	}
	name = json_object_get_string_member (object, "name");
	url = json_object_get_string_member (object, "url");
	if (NULL == name || NULL == url) {
		glib_log_warning ("Get 'name' || 'url' value err");
		return eMSG_JSON_PLAY_GET_MEMBER_ERR;
	}
	glib_log_debug("play url:%s", url);

	//2.插入视频信息，开始启动管道
	key = (gchar *)g_malloc0(strlen(name)+1);
	if (NULL == key) {
		glib_log_warning ("Key info malloc err");
		ret = eMSG_JSON_PLAY_KEY_MALLOC_ERR;
		goto ERR;
	}
	strncpy(key, name, strlen(name));
	//查询该名称通道是否存在
	if (TRUE == g_hash_table_lookup_extended (g_hash_video_info, key, \
					NULL, NULL)) {
		glib_log_warning ("Key already exist err");
		ret = eMSG_JSON_PLAY_KEY_ALREADY_EXIST;
		goto ERR;
	}

	video = (VideoInfo *)g_malloc0(sizeof(VideoInfo));
	if (NULL == video) {
		glib_log_warning ("Video info malloc err");
		goto ERR;
	}
	video->name = (gchar *)g_malloc0(strlen(name)+1);
	if (NULL == video->name) {
		glib_log_warning ("Name info malloc err");
		ret = eMSG_JSON_PLAY_VIDEO_MALLOC_ERR;
		goto ERR;
	}
	strncpy(video->name, name, strlen(name));
	video->url = (gchar *)g_malloc0(strlen(url)+1);
	if (NULL == video->url) {
		glib_log_warning ("Url info malloc err");
		ret = eMSG_JSON_PLAY_VIDEO_MALLOC_ERR;
		goto ERR;
	}
	strncpy(video->url, url, strlen(url));
	video->status = eVIDEO_STATUS_INIT;
	///开启视频通道
	info.location = url;
	video->pipe = start_pipeline(&info, app->loop);
	if (NULL != video->pipe) {
		video->status = eVIDEO_STATUS_PLAY;
		if (TRUE == g_hash_table_insert(g_hash_video_info, key, video)) {
			glib_log_info("Video start success:[%s]%s", video->name, video->url);
			ret = eMEDIA_SERVER_SUCCESS;
		} else {
			//此处应该停止视频管道
			clean_up(video->pipe);
			video->pipe = NULL;
			ret = eMSG_JSON_PLAY_HASH_INSTALL_ERR;
		}
	} else {
		ret = eMSG_JSON_PLAY_START_VIDEO_ERR;
	}
	//3.回复执行情况
ERR:
	if (ret != eMEDIA_SERVER_SUCCESS) {
		if (NULL != key) {
			g_free(key);
		}
		if (NULL != video) {
			if (NULL != video->url) g_free(video->url);
			if (NULL != video->name) g_free(video->name);
			g_free(video);
		}
		return reply_text_msg_on_json(reply, "result", "failed");
	}
	return reply_text_msg_on_json(reply, "result", "success");
}



/*********************************************************************************************
 * Description: play命令解析                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gint msg_requst_type_stop(gpointer msg_data, gchar **reply, gpointer user_data)
{
	g_assert (NULL != msg_data && NULL != reply && NULL != user_data);

	gboolean	found	= FALSE;
	const gchar	*name 	= NULL;
	gpointer	*key 	= NULL;
	gint		ret 	= eMEDIA_SERVER_SUCCESS;
	JsonObject  *object = msg_data;
	AppContext  *app 	= user_data;
	RTSPServerInfo info = {0};
	VideoInfo 	*video = NULL;

	//1.查询需要的字段并获取
	if (!json_object_has_member (object, "name")) {
		glib_log_warning ("Received message without 'name'");
		ret = eMSG_JSON_STOP_FIND_MEMBER_ERR;
		goto ERR;
	}
	name = json_object_get_string_member (object, "name");
	if (NULL == name) {
		glib_log_warning ("Get 'name' value err");
		ret = eMSG_JSON_STOP_GET_MEMBER_ERR;
		goto ERR;
	}

	//2.关闭视频，删除HashMap中的
	video = (VideoInfo *)g_hash_table_lookup(g_hash_video_info, name);
	if (NULL != video) {
		clean_up(video->pipe);
		video->pipe = NULL;
		found = g_hash_table_remove(g_hash_video_info, name);
		if (TRUE != found) {
			glib_log_warning ("Hash remove %s err", name);
			ret = eMSG_JSON_STOP_HASH_REMOVE_ERR;
		} else {
			ret = eMEDIA_SERVER_SUCCESS;
		}
	} else {
		glib_log_warning ("Hash value is NULL");
		ret = eMSG_JSON_STOP_HASH_VLAUE_ERR;
	}

ERR:
	if (ret != eMEDIA_SERVER_SUCCESS) {
		return reply_text_msg_on_json(reply, "result", "failed");
	}
	return reply_text_msg_on_json(reply, "result", "success");
}

/*********************************************************************************************
 * Description: 仅支持特定的回复命令                                                            *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static gint reply_text_msg_on_json(gchar **reply, const gchar *member, const gchar *value)
{
	g_assert (NULL != reply && NULL != member && NULL != value);
	gint ret = eMEDIA_SERVER_SUCCESS;

	JsonGenerator *gen = NULL;
	JsonNode 	  *root= NULL;

	/* 1.创建JSON */
	JsonBuilder *builder = json_builder_new ();
	if (NULL == builder) {
		glib_log_warning ("Build  JsonBuilder err");
		return eMSG_JSON_REPLY_BUILD_ERR;
	}
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, member);
	json_builder_add_string_value(builder, value);
	json_builder_end_object(builder);

	/* 2.将JSON转化为字符串 */
	gen = json_generator_new();
	if (NULL == gen) {
		g_object_unref (builder);
		return eMSG_JSON_GENERATOR_NEW_ERR;
	}
	root = json_builder_get_root(builder);
	if (NULL == root) {
		g_object_unref (gen);
		g_object_unref (builder);
		return eMSG_JSON_BUILD_GET_ROOT_ERR;
	}
	json_generator_set_root(gen, root);
	*reply = json_generator_to_data(gen, NULL);
	if (NULL == *reply) {
		ret = eMSG_JSON_GENERATOR_TO_DATA_ERR;
	}

	/* 3.清理资源 */
	json_node_free (root);
	g_object_unref (gen);
	g_object_unref (builder);

	return ret;
}


/************************************ HTTP 方式 *********************************************************/

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void do_get (SoupServer *server, SoupMessage *msg, const char *path) {
  char *slash;
  GStatBuf st;

  if (msg->method == SOUP_METHOD_GET) {
    // SoupBuffer *buffer;
    SoupMessageHeadersIter iter;
    const char *name, *value;

    /* mapping = g_mapped_file_new (path, FALSE, NULL);
    if (!mapping) {
      soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
      return;
    } */

    soup_message_headers_append (msg->response_headers,"Access-Control-Allow-Origin", "*");

    /* buffer = soup_buffer_new_with_owner (g_mapped_file_get_contents (mapping),
				 g_mapped_file_get_length (mapping),
				 mapping, (GDestroyNotify)g_mapped_file_unref);

    soup_message_body_append_buffer (msg->response_body, buffer); */

    // soup_buffer_free (buffer);
  } else /* msg->method == SOUP_METHOD_HEAD */ {
    char *length;

    /* We could just use the same code for both GET and
     * HEAD (soup-message-server-io.c will fix things up).
     * But we'll optimize and avoid the extra I/O.
     */
    length = g_strdup_printf ("%lu", (gulong)st.st_size);
    soup_message_headers_append (msg->response_headers, "Content-Length", length);
    g_free (length);
  }
  soup_message_set_status (msg, SOUP_STATUS_OK);
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void do_post (SoupServer *server, SoupMessage *msg) {
#if 0
  SoupBuffer *buffer;
  gchar *text;
  JsonObject *json_obj;
  GError **error;

  json_obj = json_object_new ();
  json_object_set_string_member (json_obj, "code", "0");
  text = json_to_string(json_obj, true);
  json_object_unref (json_obj);

  buffer = soup_buffer_new_with_owner (text, strlen(text), text, (GDestroyNotify)g_free);

  soup_message_headers_append (msg->response_headers,"Access-Control-Allow-Origin", "*");
  soup_message_body_append_buffer (msg->response_body, buffer);
  soup_buffer_free (buffer);

  soup_message_set_status (msg, SOUP_STATUS_OK);
#endif

  if (msg->request_body->length) {
    g_print ("%s\n", msg->request_body->data);

    //===============add by liyujia, 20180905=================
    //now ,parse json data
    JsonObject *object = parse_json_object(msg->request_body->data);

#if 0
    if (json_object_has_member (object, "ConnectToServer")) {
      JsonObject *child;
      const gchar *server_url_, *terminal_id_;
      child = json_object_get_object_member (object, "ConnectToServer");
      if (json_object_has_member (child, "server_url") && json_object_has_member (child, "terminal_id")) {
        server_url_ = json_object_get_string_member (child, "server_url");
        terminal_id_ = json_object_get_string_member (child, "terminal_id");
      }
      g_print ("server_url = %s,  terminal_id = %s\n", server_url_, terminal_id_);

      // checkAndConnect(server_url_, terminal_id_);
    } else if (json_object_has_member (object, "Monitoring")) {
      JsonObject *child;
      const gchar *_event_, *_data_, *_peerId_;
      child = json_object_get_object_member (object, "Monitoring");

      if (json_object_has_member (child, "_event") && json_object_has_member (child, "_data") && json_object_has_member (child, "_peerId")) {
        _event_ = json_object_get_string_member (child, "_event");
        _data_ = json_object_get_string_member (child, "_data");
        _peerId_ = json_object_get_string_member (child, "_peerId");
      }
      g_print ("event = %s,  data = %s, peerId = %s\n", _event_, _data_, _peerId_);

      if (!setup_call (_peerId_)) {
        g_print ("Failed to setup call!\n");
        cleanup_and_quit_loop ("ERROR: Failed to setup call", PEER_CALL_ERROR);
      }

      } else {
        g_print ("do nothing!\n");
      }
      //add end
#endif
  }

}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void do_put (SoupServer *server, SoupMessage *msg, const char *path) {
  GStatBuf st;
  FILE *f;
  gboolean created = TRUE;

  if (g_stat (path, &st) != -1) {
    const char *match = soup_message_headers_get_one (msg->request_headers, "If-None-Match");
    if (match && !strcmp (match, "*")) {
      soup_message_set_status (msg, SOUP_STATUS_CONFLICT);
      return;
    }
    if (!g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
      soup_message_set_status (msg, SOUP_STATUS_FORBIDDEN);
      return;
    }
    created = FALSE;
  }

  f = fopen (path, "w");
  if (!f) {
    soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);
    return;
  }

  fwrite (msg->request_body->data, 1, msg->request_body->length, f);
  fclose (f);
  soup_message_set_status (msg, created ? SOUP_STATUS_CREATED : SOUP_STATUS_OK);
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void http_callback (SoupServer *server, SoupMessage *msg,
		 const char *path, GHashTable *query,
		 SoupClientContext *context, gpointer data) {
  char *file_path;
  SoupMessageHeadersIter iter;
  const char *name, *value;

  g_info ("%s %s HTTP/1.%d\n", msg->method, path, soup_message_get_http_version (msg));
  soup_message_headers_iter_init (&iter, msg->request_headers);

  while (soup_message_headers_iter_next (&iter, &name, &value))
    g_info ("%s: %s\n", name, value);

  file_path = g_strdup_printf (".%s", path);

  if (msg->method == SOUP_METHOD_GET || msg->method == SOUP_METHOD_HEAD)
    do_get (server, msg, file_path);
  else if (msg->method == SOUP_METHOD_PUT)
    do_put (server, msg, file_path);
  else if (msg->method == SOUP_METHOD_POST)
    do_post(server, msg);
  else
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);

  g_free (file_path);
  g_info ("  -> %d %s\n\n", msg->status_code, msg->reason_phrase);
}
