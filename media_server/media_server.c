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

#include "sys.h"
#include "media_server.h"
#include "utils.h"
#include "gst_rtsp.h"

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
	gchar 		*user_id;
	gchar 		*user_pwd;
	VideoStatus status;
	RtspPipelineBundle 	*pipe;
}VideoInfo;
//RTCP控制数据写入文件
#define FILE_PATH_RUNTIME		"/runtime/"
#define FILE_PATH_HISTORY		"/history/"
#define FILE_PATH_HISTORY_LIST	"/list/"

#define FILE_HISTORY_LIST_VALUE "{\"Files\":[]}"
#define FILE_HISTORY_VALUE 		"{\"data\":[]}"

typedef void (*HashForeach)(gpointer key, gpointer value, gpointer user_data);

typedef struct {
	GString *process_name;
	GString	*runtime_file;
	GString	*history_file;
	GString	*history_list_file;

	GDateTime  	*time;
	gint64		time_sec;

	JsonParser	*history;	//历史数据
	JsonArray	*history_data_array;	//历史文件数据节点
}RtcpToFile;


void stop_server(SoupServer* server);

/*******************************************************************************************************/
static gint init_rtcp_file(RtcpToFile *rtcp, const gchar *result_path, const gchar *task_name);
static gint add_rtcp_file_list(RtcpToFile *rtcp);
//内部使用函数
static void websocket_callback (SoupServer *server, SoupWebsocketConnection *connection,
								const char *path, SoupClientContext *client, gpointer user_data);
static gboolean send_rtcp_callback (gpointer user_data);
static JsonNode* add_rtcp_data_to_json(HashForeach hash_foreach_call);
static void hash_tab_foreach_runtime_call(gpointer key, gpointer value, gpointer user_data);
static void hash_tab_foreach_history_call(gpointer key, gpointer value, gpointer user_data);
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

static GHashTable *g_hash_video_info = NULL;	//HashMap存储视频信息
static RtcpToFile g_rtcp_to_file = {0};			//处理rtcp信息到文件中

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
SoupServer* start_server(AppOption *opt, AppContext *app)
{

	if (NULL == opt || NULL == app) {
		return NULL;
	}
	GSList *uris = NULL, *u = NULL;
	GTlsCertificate *cert = NULL;
	gchar *str = NULL;
	GError *error = NULL;
	SoupServer *server = NULL;
	gint	ret = eMEDIA_SERVER_SUCCESS;
	gboolean listen_flage = FALSE;

	/* 1.创建视频信息存储HashMap */
	g_hash_video_info = g_hash_table_new_full(g_str_hash, g_str_equal, \
			hash_remove_key_call, hash_remove_value_call);
	if (NULL == g_hash_video_info) {
		g_error ("Creat video hashmap err\n");
		return NULL;
	}

	/* 2.创建监听服务 */
	if ( opt->tls_cert_file && opt->tls_key_file ) {
		cert = g_tls_certificate_new_from_files (opt->tls_cert_file, opt->tls_key_file, &error);
		if (error) {
			  g_error ("Unable to create server: %s\n", error->message);
			  g_error_free(error);
			  stop_server(NULL);
			  return NULL;
		}
		server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "simple-httpd ",
								 SOUP_SERVER_TLS_CERTIFICATE, cert, NULL);
		g_object_unref (cert);
		listen_flage = soup_server_listen_local (server, opt->listen_port, SOUP_SERVER_LISTEN_HTTPS, &error);
	} else {
		server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "simple-httpd ", NULL);
		listen_flage = soup_server_listen_local (server, opt->listen_port, 0, &error);
	}
	if (FALSE == listen_flage) {
		stop_server(NULL);
		return NULL;
	}

	/* 3.增加WebSocket处理函数 */
	soup_server_add_websocket_handler(server, NULL, NULL, NULL,
								   websocket_callback,
								   app, NULL);
	uris = soup_server_get_uris (server);
	for (u = uris; u; u = u->next) {
		str = soup_uri_to_string (u->data, FALSE);
		glib_log_debug ("Listening on %s", str);
		g_free (str);
		soup_uri_free (u->data);
	}
	g_slist_free (uris);
	glib_log_debug ("Waiting for requests...");
	//打开Gstream
	gstream_init();

	/* 4.创建定时任务去发送RTCP的数据 */
	ret = init_rtcp_file(&g_rtcp_to_file, opt->result_file_path, opt->task_name);
	if (eMEDIA_SERVER_SUCCESS != ret) {
		glib_log_warning("init_rtcp_file err:%d", ret);
		stop_server(server);
		return NULL;
	}
	GSource *source = g_timeout_source_new (1000);
	guint sourceID = g_source_attach (source, NULL);
	glib_log_debug("source_get_ID:%d\n", sourceID);
	g_source_set_callback (source, send_rtcp_callback, app, NULL);

	return server;
}

/*********************************************************************************************
 * Description: 停止websocket服务， 释放资源                                                    *
 *                                                                                           *
 * Input : SoupServer* server:停止服务的指针                                                   *
 * Return: 无                                                                                *
 *********************************************************************************************/
void stop_server(SoupServer* server)
{
	//停止服务
	if (NULL != server) {
		glib_log_debug("Stop server...");
		soup_server_disconnect (server);
	}
	//释放hashmap的值
	if (NULL != g_hash_video_info) {
		//g_hash_table_foreach_remove(g_hash_video_info, hash_table_sub_remove_call, NULL);
		g_hash_table_destroy (g_hash_video_info);
	}
	//释放rtcp写入文件的信息体
	if (NULL != g_rtcp_to_file.time)			g_date_time_unref(g_rtcp_to_file.time);
	if (NULL != g_rtcp_to_file.process_name)	g_string_free(g_rtcp_to_file.process_name, TRUE);
	if (NULL != g_rtcp_to_file.runtime_file)	g_string_free(g_rtcp_to_file.runtime_file, TRUE);
	if (NULL != g_rtcp_to_file.history_file)	g_string_free(g_rtcp_to_file.history_file, TRUE);
	if (NULL != g_rtcp_to_file.history_list_file)	g_string_free(g_rtcp_to_file.history_list_file, TRUE);
	if (NULL != g_rtcp_to_file.history) 			g_object_unref(g_rtcp_to_file.history);
}

/*********************************************************************************************
 * Description: 初始化rtcp参数写入文件的参数，创建文件	                                         *                                                                                     *
 * Input : 	RtcpToFile *rtcp: RTCP:写入文件的的信息缓存区										 *
 * 			const char *task_name: 测试任务名                                         		 *
 * Return:  创建文件的结果																		 *
 * 			eMEDIA_SERVER_SUCCESS:成功														 *
 * 			其他：失败                                                                         *
 *********************************************************************************************/
static gint init_rtcp_file(RtcpToFile *rtcp, const gchar *result_path, const gchar *task_name)
{
	g_assert(NULL != rtcp && NULL != result_path && NULL != task_name);

	gchar 	date[128] = {0};
	gint	ret = eMEDIA_SERVER_SUCCESS;

	/* 1.申请空间 */
	gchar *path = g_strdup (result_path);
	if (NULL == path) {
		return eMEDIA_SERVER_INVALID;
	}
	glib_log_debug("Result file path:%c", path[strlen(path)-1]);
	if ('/' == path[strlen(path)-1]) {
		path[strlen(path)-1] = '\0';
	}
	glib_log_debug("Result file path:%s", path);

	rtcp->process_name = g_string_new(NULL);
	rtcp->runtime_file = g_string_new(path);
	rtcp->history_file = g_string_new(path);
	rtcp->history_list_file = g_string_new(path);
	g_free(path);
	if (NULL == rtcp->process_name \
		|| NULL == rtcp->runtime_file \
		|| NULL == rtcp->history_file \
		|| NULL == rtcp->history_list_file) {
		return eMSG_RTCP_FILE_INIT_ERR;
	}
	rtcp->runtime_file = g_string_append (rtcp->runtime_file, FILE_PATH_RUNTIME);
	rtcp->history_file = g_string_append (rtcp->history_file, FILE_PATH_HISTORY);
	rtcp->history_list_file = g_string_append (rtcp->history_list_file, FILE_PATH_HISTORY_LIST);

	/* 2.创建文件 */
	if (0 != get_process_name(rtcp->process_name)) {
		return eMSG_RTCP_FILE_GET_PEOCESS_NAME_ERR;
	}
	glib_log_debug("process name:%s", rtcp->process_name->str);

	rtcp->time = g_date_time_new_now_local();
	if (NULL == rtcp->time) {
		return eMSG_RTCP_FILE_GET_TIME_ERR;
	}

	//拷贝进程名称
	rtcp->runtime_file = g_string_append (rtcp->runtime_file, rtcp->process_name->str);
	rtcp->history_file = g_string_append (rtcp->history_file, rtcp->process_name->str);
	rtcp->history_list_file = g_string_append (rtcp->history_list_file, rtcp->process_name->str);
	//拷贝后缀名
	rtcp->runtime_file = g_string_append (rtcp->runtime_file, ".json");
	rtcp->history_list_file = g_string_append (rtcp->history_list_file, "_list.json");
	//拼接历史数据的后缀名
	sprintf(date, "_%s_%d%02d%02d_%02d%02d%02d.json", task_name, g_date_time_get_year(rtcp->time), g_date_time_get_month(rtcp->time), \
			g_date_time_get_day_of_month(rtcp->time), g_date_time_get_hour(rtcp->time),  g_date_time_get_minute(rtcp->time), \
			g_date_time_get_second(rtcp->time));
	rtcp->history_file = g_string_append(rtcp->history_file, date);
	glib_log_debug("runtime:%s", rtcp->runtime_file->str);
	glib_log_debug("history_file:%s", rtcp->history_file->str);
	glib_log_debug("history_list_file:%s", rtcp->history_list_file->str);

	/* 3.检查文件是否存在，不存在则创建 */
	//增加历史文件
	glib_log_debug("Create %s start.", rtcp->history_file->str);
	ret = open_file(rtcp->history_file->str, "w+", FILE_HISTORY_VALUE,\
					strlen(FILE_HISTORY_VALUE));
	if (aWork_FILE_SUCCESS != ret) {
		return eMSG_RTCP_FILE_WRITE_LIST_ERR;
	}
	JsonNode *root = get_root_node_from_file(&(rtcp->history), rtcp->history_file->str);
	if (NULL == root) {
		return eMSG_RTCP_FILE_PARSE_LOAD_ERR;
	}
	rtcp->history_data_array = get_array_from_node(root, "data");
	if (NULL == rtcp->history_data_array) {
		return eMSG_RTCP_FILE_GET_DATA_ERR;
	}
	glib_log_debug("Create %s end.", rtcp->history_file->str);

	//检查历史列表文件是否存在
	glib_log_debug("Create %s start.", rtcp->history_list_file->str);
	GFile * list_file = g_file_new_for_path (rtcp->history_list_file->str);
	if (FALSE == g_file_query_exists (list_file, NULL)) {
		ret = open_file(rtcp->history_list_file->str, "w+", FILE_HISTORY_LIST_VALUE,\
				strlen(FILE_HISTORY_LIST_VALUE));
		if (aWork_FILE_SUCCESS != ret) {
			g_object_unref(list_file);
			return eMSG_RTCP_FILE_WRITE_LIST_ERR;
		}
	}
	g_object_unref(list_file);
//	if (0 != access(rtcp->history_list_file->str, F_OK)) {
//		ret = open_file(rtcp->history_list_file->str, "w+", FILE_HISTORY_LIST_VALUE,\
//				strlen(FILE_HISTORY_LIST_VALUE));
//		if (aWork_FILE_SUCCESS != ret) {
//			return eMSG_RTCP_FILE_WRITE_LIST_ERR;
//		}
//	}
	//增加历史列表
	ret = add_rtcp_file_list(rtcp);
	if (eMEDIA_SERVER_SUCCESS != ret) {
		return ret;
	}
	glib_log_debug("Create %s end.", rtcp->history_list_file->str);

	return eMEDIA_SERVER_SUCCESS;
}
/*********************************************************************************************
 * Description: 向历史列表文件中增加节点                                                         *                                                                                     *
 * Input :     RtcpToFile *rtcp: RTCP:写入文件的的信息缓存区                                     *
 * Return:     eMEDIA_SERVER_SUCCESS：成功	其他：失败                                         *
 *********************************************************************************************/
static gint add_rtcp_file_list(RtcpToFile *rtcp)
{
	g_assert(NULL != rtcp);
	gint		ret = eMEDIA_SERVER_SUCCESS;
	GError 		*error = NULL;
	JsonParser 	*parser = NULL;

	JsonNode *root = get_root_node_from_file(&parser, rtcp->history_list_file->str);
	if (NULL == root) {
		return eMSG_RTCP_FILE_PARSE_LOAD_ERR;
	}
	JsonArray *files = get_array_from_node(root, "Files");
	if (NULL == files) {
		glib_log_warning ("History list no 'Files'");
		ret = eMSG_RTCP_FILE_GET_ARRAY_ERR;
		goto ERR;
	}

	//整理需要增加的节点
	JsonObject *add_object = json_object_new();
	if (NULL == add_object) {
		ret = eMSG_RTCP_FILE_GET_ARRAY_ERR;
		goto ERR;
	}

	gchar *file_name = g_path_get_basename(rtcp->history_file->str);
	if (NULL == file_name) {
		ret = eMSG_RTCP_FILE_GET_FILE_NAME_ERR;
		goto ERR;
	}
	json_object_set_string_member(add_object, "FileName", file_name);
	json_object_set_int_member(add_object, "StartTime", g_date_time_to_unix(rtcp->time)*1000);
	g_free(file_name);

	json_array_add_object_element (files, add_object);

	JsonGenerator *gen = json_generator_new();
	if (NULL == gen) {
		ret = eMSG_RTCP_FILE_GET_GEN_ERR;
		goto ERR;
	}
	json_generator_set_root(gen, root);
	if (TRUE == json_generator_to_file (gen, g_rtcp_to_file.history_list_file->str, &error)) {
		glib_log_debug("History list file write OK!!!");
	} else {
		 g_error_free(error);
	}

ERR:
	g_object_unref (gen);
	g_object_unref(parser);
	return ret;
}
/*********************************************************************************************
 * Description: 整理视频状态推送结构                                                 			 *
 *                                                                                           *
 * Input : gpointer user_data：回到函数的参数                                                  *
 * Return:  TRUE：定时器继续运行	FALSE：定时器结束                                              *
 *********************************************************************************************/
static gboolean send_rtcp_callback (gpointer user_data)
{
	if (NULL == user_data) {
		glib_log_info("send_rtcp_callback data is NULL");
		return TRUE;
	}
	gint i = 0;
	GError *err = NULL;
	gint ret = eMEDIA_SERVER_SUCCESS;
	JsonNode 	  *root= NULL, *root_history = NULL;
	AppContext 	*app = user_data;

	if (NULL == app->server || NULL == g_hash_video_info
		|| 0 == g_hash_table_size(g_hash_video_info)) {
		return TRUE;
	}

	/* 1.创建JSON */
	//获取时间戳
	g_rtcp_to_file.time = g_date_time_new_now_local();
	if (NULL == g_rtcp_to_file.time) {
		return eMSG_RTCP_FILE_GET_TIME_ERR;
	}

	/* 2.写入实时数据 */
	root = add_rtcp_data_to_json(hash_tab_foreach_runtime_call);
	if (NULL != root) {
		ret = write_json_to_file(g_rtcp_to_file.runtime_file->str, root);
		if (0 != ret) {
			glib_log_warning("Runtime file write err:%d", ret);
		}
		json_node_free (root);
		root = NULL;
	} else {
		glib_log_warning("Runtime json add data err");
	}

	/* 3.写入历史数据 */
	root = add_rtcp_data_to_json(hash_tab_foreach_history_call);
	if (NULL != root) {
		JsonObject *add_object = json_node_dup_object(root);
		if (NULL == add_object) {
			json_node_free (root);
			return TRUE;
		}
		json_array_add_object_element (g_rtcp_to_file.history_data_array, add_object);
		root_history = json_parser_get_root (g_rtcp_to_file.history);	/* 返回的节点属于g_rtcp_to_file.history 所有，不应释放资源 */
		if (NULL == root_history) {
			json_node_free (root);
			return TRUE;
		}
		ret = write_json_to_file(g_rtcp_to_file.history_file->str, root_history);
		if (0 != ret) {
			glib_log_warning("History file write err:%d", ret);
		}
		json_node_free (root);
	} else {
		glib_log_warning("History json add data err");
	}

	return TRUE;	//此处必须返回TRUE，否则不循环
}

/*********************************************************************************************
 * Description: 增加数据到JSON中                                                 			 	 *
 *                                                                                           *
 * Input : HashForeach hash_foreach_call:Hash 遍历的循环函数                                   *
 * Return:  Json 增加的节点                                              *
 *********************************************************************************************/
static JsonNode* add_rtcp_data_to_json(HashForeach hash_foreach_call)
{
	g_assert(NULL != hash_foreach_call);

	JsonNode 	  *root= NULL;
	JsonBuilder *builder = json_builder_new ();
	if (NULL == builder) {
		glib_log_warning ("Build JsonBuilder err");
		return NULL;
	}

	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "TimeStamp");
	//json_builder_add_int_value(builder, g_get_real_time()/1000);
	json_builder_add_int_value(builder, g_date_time_to_unix(g_rtcp_to_file.time)*1000);

	gchar *time_str = g_date_time_format(g_rtcp_to_file.time, "%Y/%m/%d %T");
	if (NULL != time_str) {
		json_builder_set_member_name(builder, "CurrentTime");
		json_builder_add_string_value(builder, time_str);
		g_free(time_str);
	}
	//增加变量
	g_hash_table_foreach(g_hash_video_info,	hash_foreach_call, builder);
	json_builder_end_object(builder);

	root = json_builder_get_root(builder);	/* 释放返回的值json_node_unref()。 */
	if (NULL == root) {
		g_object_unref (builder);
		return NULL;
	}
	g_object_unref (builder);
	return root;
}

/*********************************************************************************************
 * Description: Hash中的视频实时参数遍历回调函数                                                	 *
 *                                                                                           *
 * Input : 	gpointer key：Hash 的key值
 * 			gpointer value：Hash 的Vlaue
 * 			gpointer user_data：回调传入的参数                                                 *
 * Return:  无                                                                               *
 *********************************************************************************************/
static void hash_tab_foreach_runtime_call(gpointer key, gpointer value, gpointer user_data)
{
	guint i = 0;
	JsonBuilder *builder = (JsonBuilder *)user_data;
	VideoInfo *info = (VideoInfo *)value;
	if (NULL == builder || NULL == info) {
		glib_log_warning("value is NULL");
		return;
	}
	glib_log_debug("key:%s", (gchar*)key);

	for (i = 0; NULL != value && i < RTCP_MAX_NUM; i++) {
		if (TRUE != g_rtcp_parameter[i].write_history_flage) {
			continue;
		}
		json_builder_set_member_name(builder, g_rtcp_parameter[i].key);
		switch (g_rtcp_parameter[i].value_type) {
		 case eBollean: {
			 json_builder_add_boolean_value(builder, info->pipe->RtcpParseValue[i].value_bool);
		  } break;
		  case eInt: {
			  json_builder_add_int_value(builder, info->pipe->RtcpParseValue[i].value_int);
		  } break;
		  case eUint: {
			  if (g_rtcp_parameter[i].id == eSentRbJitter) {
				  gint jitter = (0 != info->pipe->RtcpParseValue[eClockRate].value_int)? \
						  info->pipe->RtcpParseValue[i].value_uint/info->pipe->RtcpParseValue[eClockRate].value_int : 0;
				  json_builder_add_int_value(builder, jitter);
			  } else {
				  json_builder_add_int_value(builder, info->pipe->RtcpParseValue[i].value_uint);
			  }

		  } break;
		  case eInt64: {
			  json_builder_add_int_value(builder, info->pipe->RtcpParseValue[i].value_int64);
		  } break;
		  case eUint64: {
			  json_builder_add_int_value(builder, info->pipe->RtcpParseValue[i].value_uint64);
		  } break;
		}
	}
	return;
}

/*********************************************************************************************
 * Description: Hash中的视频历史参数遍历回调函数                                                	 *
 *                                                                                           *
 * Input : 	gpointer key：Hash 的key值
 * 			gpointer value：Hash 的Vlaue
 * 			gpointer user_data：回调传入的参数                                                 *
 * Return:  无                                                                               *
 *********************************************************************************************/
static void hash_tab_foreach_history_call(gpointer key, gpointer value, gpointer user_data)
{
	guint i = 0;
	JsonBuilder *builder = (JsonBuilder *)user_data;
	VideoInfo *info = (VideoInfo *)value;

	if (NULL == builder || NULL == info) {
		glib_log_warning("value is NULL");
		return;
	}
	glib_log_debug("key:%s", (gchar*)key);

	for (i = 0; NULL != value && i < RTCP_MAX_NUM; i++) {
		json_builder_set_member_name(builder, g_rtcp_parameter[i].key);
		switch (g_rtcp_parameter[i].value_type) {
		 case eBollean: {
			 json_builder_add_boolean_value(builder, info->pipe->RtcpParseValue[i].value_bool);
		  } break;
		  case eInt: {
			  json_builder_add_int_value(builder, info->pipe->RtcpParseValue[i].value_int);
		  } break;
		  case eUint: {
			  if (g_rtcp_parameter[i].id == eSentRbJitter) {
				  gint jitter = (0 != info->pipe->RtcpParseValue[eClockRate].value_int)? \
						  info->pipe->RtcpParseValue[i].value_uint/info->pipe->RtcpParseValue[eClockRate].value_int : 0;
				  json_builder_add_int_value(builder, jitter);
			  } else {
				  json_builder_add_int_value(builder, info->pipe->RtcpParseValue[i].value_uint);
			  }
		  } break;
		  case eInt64: {
			  json_builder_add_int_value(builder, info->pipe->RtcpParseValue[i].value_int64);
		  } break;
		  case eUint64: {
			  json_builder_add_int_value(builder, info->pipe->RtcpParseValue[i].value_uint64);
		  } break;
		}
	}
	return;
}

/*********************************************************************************************
 * Description: 释放HashMap资源，回调函数                                                       *
 *                                                                                           *
 * Input : 	gpointer key：Hash 的key值
 * 			gpointer value：Hash 的Vlaue
 * 			gpointer user_data：回调传入的参数                                                 *
 * Return:  是否移除成功                                                                       *
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
		if (NULL != video->user_id) 	g_free(video->user_id);
		if (NULL != video->user_pwd) 	g_free(video->user_pwd);
		if (NULL != video->pipe)	{
			clean_up(video->pipe);
			video->pipe = NULL;
		}
		g_free(video);
	}
	return TRUE;
}
/*********************************************************************************************
 * Description: 释放HashMap的Key资源，回调函数                                                  *
 *                                                                                           *
 * Input : gpointer data：回调传入的指针（Key的指针）                                            *
 * Return:  是否移除成功                                                                       *
 *********************************************************************************************/
static void hash_remove_key_call (gpointer data)
{
	gchar *key = (gchar *)data;
	if (NULL != key) {
		glib_log_info("Remove key:%s", (gchar*)key);
		g_free(key);
	}
}
/*********************************************************************************************
 * Description: 释放HashMap的Value资源，回调函数                                                  *
 *                                                                                           *
 * Input : gpointer data：回调传入的指针（Value的指针）                                            *
 * Return:  是否移除成功                                                                       *
 *********************************************************************************************/
static void hash_remove_value_call (gpointer data)
{
	VideoInfo *video = (VideoInfo *)data;
	if (NULL != video) {
		glib_log_info("Remove value:%s", video->name);
		if (NULL != video->url) 	g_free(video->url);
		if (NULL != video->name) 	g_free(video->name);
		if (NULL != video->user_id) 	g_free(video->user_id);
		if (NULL != video->user_pwd) 	g_free(video->user_pwd);
		if (NULL != video->pipe)	{
			clean_up(video->pipe);
			video->pipe = NULL;
		}
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
 * Input : 	 oupServer *server：服务
 * 			 SoupWebsocketConnection *connection：链接
     	 	 const char *path：路径
     	 	 SoupClientContext *client：客户端
     	 	 gpointer user_data：回调传入的数据                                                *
 * Return:   无                                                                                *
 *********************************************************************************************/
static void websocket_callback (SoupServer *server, SoupWebsocketConnection *connection,
     const char *path, SoupClientContext *client, gpointer user_data ) {

	GBytes *received = NULL;
	glib_log_info("websocket connected");

	AppContext *ctx = user_data;
	ctx->server = g_object_ref (connection);
	g_signal_connect (ctx->server, "message", G_CALLBACK (on_message), &received);

	return;
}

/*********************************************************************************************
 * Description: 处理消息                                                                      *
 *                                                                                           *
 * Input : 	SoupWebsocketConnection *conn：链接
 * 			gint type：信息类型
 * 			GBytes *message：信息
 * 			gpointer data：回调函数传入的参数                                                   *
 * Return:  无                                                                               *
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
 * Input :	SoupWebsocketConnection *ws：链接
 * 			gint type：信息类型
 * 			GBytes *message：信息
 * 			gpointer data：回调函数传入的参数 													 *
 * Return:  eMEDIA_SERVER_SUCCESS:成功	其他：失败                                            *
 *********************************************************************************************/
static gint on_text_message (SoupWebsocketConnection *ws,
                 SoupWebsocketDataType type,
                 GBytes *message,
                 gpointer user_data) {
	g_assert (ws != NULL && message != NULL && user_data != NULL);
	g_assert_cmpint (type, ==, SOUP_WEBSOCKET_DATA_TEXT);

	gint 		ret		= eMEDIA_SERVER_SUCCESS;
	gsize 		length 	= 0;
	const gchar *msg_ptr = NULL;
	gchar *msg_reply 	= NULL;

	//1.获取字符
	msg_ptr = g_bytes_get_data(message, &length);
	if (NULL == msg_ptr || 0 == length) {
		glib_log_warning("Get text err");
		return eMSG_GET_ERR;
	}
	glib_log_debug("Recv[%ld]:%s", length, msg_ptr);

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
 * Description: 处理二进制信息(暂时不用)                                                        *
 *                                                                                           *
 * Input :	SoupWebsocketConnection *ws：链接
 * 			gint type：信息类型
 * 			GBytes *message：信息
 * 			gpointer data：回调函数传入的参数 													 *
 * Return:  0:成功	其他：失败                                            					 *
 *********************************************************************************************/
static gint on_binary_message (SoupWebsocketConnection *ws,
		   SoupWebsocketDataType type,
		   GBytes *message,
		   gpointer user_data) {
	GBytes **receive = user_data;

	g_assert_cmpint (type, ==, SOUP_WEBSOCKET_DATA_BINARY);
	g_assert (*receive == NULL);
	g_assert (message != NULL);

	return 0;
}


/*********************************************************************************************
 * Description: 以JSON形式解析字符串消息                                                        *
 *                                                                                           *
 * Input :	const gchar *msg_ptr：字符串数据
 * 			gsize length：字符串长度
 * 			gchar **msg_reply：回复数据
 * 			gpointer user_data：用户参数                                                             *
 * Return:  eMEDIA_SERVER_SUCCESS：成功	其他：失败                                            *
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
		g_error_free(error);
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
	if (NULL == object || !json_object_has_member (object, "type")) {
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
 * Description: play命令解析                                                                  *
 *                                                                                           *
 * Input :  gpointer msg_data：JSON Object
 * 			gchar **msg_reply：回复数据
 * 			gpointer user_data：用户参数                                                       *
 * Return:  返回处理结果                                                                       *
 *********************************************************************************************/
static gint msg_requst_type_paly(gpointer msg_data, gchar **reply, gpointer user_data)
{
	g_assert (NULL != msg_data && NULL != reply && NULL != user_data);

	gint		ret		= eMEDIA_SERVER_SUCCESS;
	const gchar	*name = NULL, *url = NULL, *user_id = NULL, *user_pw = NULL;
	JsonObject 	*object 	= msg_data;
	RTSPServerInfo info = {0};
	AppContext 	*app = user_data;
	gchar		*key = NULL;
	VideoInfo 	*video = NULL;
	gboolean	found = FALSE;

	//1.查询需要的字段并获取
	if (!json_object_has_member (object, "name") \
		|| !json_object_has_member (object, "url") \
		|| !json_object_has_member (object, "user_id") \
		|| !json_object_has_member (object, "user_pw")) {

		glib_log_warning ("Received message without 'name' || 'url' || 'user_id' || 'user_pw'");
		return eMSG_JSON_PLAY_FIND_MEMBER_ERR;
	}
	name = json_object_get_string_member (object, "name");
	url = json_object_get_string_member (object, "url");
	user_id = json_object_get_string_member (object, "user_id");
	user_pw = json_object_get_string_member (object, "user_pw");
	if (NULL == name || NULL == url || NULL == user_id || NULL == user_pw) {
		glib_log_warning ("Get 'name' || 'url' || 'user_id' || 'user_pwd' value err");
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
	/* if (TRUE == g_hash_table_lookup_extended (g_hash_video_info, key, \
					NULL, NULL)) {
		glib_log_warning ("Key already exist err");
		ret = eMSG_JSON_PLAY_KEY_ALREADY_EXIST;
		goto ERR;
	} */
	video = (VideoInfo *)g_hash_table_lookup(g_hash_video_info, key);
	if (NULL != video) {
		glib_log_debug("Hash remove key:%s", video->name);
		clean_up(video->pipe);
		video->pipe = NULL;
		found = g_hash_table_remove(g_hash_video_info, name);
		if (TRUE != found) {
			glib_log_warning("Hash remove %s err", name);
			ret = eMSG_JSON_STOP_HASH_REMOVE_ERR;
		}
		else {
			ret = eMEDIA_SERVER_SUCCESS;
		}
	}
	else {
		glib_log_warning("Hash value is NULL");
		ret = eMSG_JSON_STOP_HASH_VLAUE_ERR;
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
	video->user_id = (gchar *)g_malloc0(strlen(user_id)+1);
	if (NULL == video->user_id) {
		glib_log_warning ("User_id info malloc err");
		ret = eMSG_JSON_PLAY_VIDEO_MALLOC_ERR;
		goto ERR;
	}
	strncpy(video->user_id, user_id, strlen(user_id));
	video->user_pwd = (gchar *)g_malloc0(strlen(user_pw)+1);
	if (NULL == video->user_pwd) {
		glib_log_warning ("User_pwd info malloc err");
		ret = eMSG_JSON_PLAY_VIDEO_MALLOC_ERR;
		goto ERR;
	}
	strncpy(video->user_pwd, user_pw, strlen(user_pw));

	video->status = eVIDEO_STATUS_INIT;
	///开启视频通道
	info.location = video->url;
	info.user_id = video->user_id;
	info.user_pwd = video->user_pwd;
	video->pipe = start_pipeline(&info, app->loop, name);
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
			if (NULL != video->user_id) g_free(video->user_id);
			if (NULL != video->user_pwd) g_free(video->user_pwd);
			g_free(video);
		}
		return reply_text_msg_on_json(reply, "result", "failed");
	}
	return reply_text_msg_on_json(reply, "result", "success");
}

/*********************************************************************************************
 * Description: stop命令解析                                                                  *
 *                                                                                           *
 * Input :  gpointer msg_data：JSON Object
 * 			gchar **msg_reply：回复数据
 * 			gpointer user_data：用户参数                                                       *
 * Return:  返回处理结果                                                                       *
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
		glib_log_debug("Hash remove key:%s", video->name);
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
 * Input :		gchar **reply：返回字符串存储缓存区
 * 			 	const gchar *member：key
 * 			 	const gchar *value：value                                                    *
 * Return:      eMEDIA_SERVER_SUCCESS：成功
 * 				其他：失败                                                                     *
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
