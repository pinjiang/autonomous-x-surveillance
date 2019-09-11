#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

gchar* get_string_from_json_object (JsonObject * object);

JsonObject* parse_json_object(const gchar* text);
JsonArray*	get_array_from_node(JsonNode *root, gchar *name);
JsonNode*	get_root_node_from_file(JsonParser **parser, const gchar *file);
gint 		write_json_to_file(const gchar *file, JsonNode *node);

guint hash_func(gconstpointer key);

gboolean key_equal_func(gconstpointer a, gconstpointer b);

void log_handler_cb (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);

gint get_process_name(GString *name);

/// 日志输出头.线程号(任务号) + [时间]
#define CLOG_PREFIX "--%s==%s==[%d]:\r\n"

///日志结束
#if 0
	#define CLOG_END	"\r\n"
#else
	#define	CLOG_END
#endif

#if WIN32

#define glib_log_debug g_debug
#define glib_log_info g_info
#define glib_log_msg g_message
#define glib_log_warning g_warning
#define glib_log_error g_error

#else 
#  define glib_log_debug(fmt, ...) \
		g_debug("D"CLOG_PREFIX fmt CLOG_END,\
				__FILE__,__FUNCTION__, __LINE__ , ##__VA_ARGS__ )
#  define glib_log_info(fmt, ...) \
		g_info("I"CLOG_PREFIX fmt CLOG_END,\
				__FILE__,__FUNCTION__, __LINE__ , ##__VA_ARGS__ )
#  define glib_log_msg(fmt, ...) \
		g_message("M"CLOG_PREFIX fmt CLOG_END,\
				__FILE__,__FUNCTION__, __LINE__ , ##__VA_ARGS__ )
#  define glib_log_warning(fmt, ...) \
		g_warning("W"CLOG_PREFIX fmt CLOG_END,\
				__FILE__,__FUNCTION__, __LINE__ , ##__VA_ARGS__ )
#  define glib_log_error(fmt, ...) \
		g_error("E"CLOG_PREFIX fmt CLOG_END,\
				__FILE__,__FUNCTION__, __LINE__ , ##__VA_ARGS__ )
#endif


//定义的返回结果
enum {
	aWork_FILE_SUCCESS = 0,
	aWork_FILE_FAILE   = -1,
	aWork_FILE_INVALID = -2,

	aWork_FILE_OPEN_FILE_ERR = -3,
	aWork_FILE_READ_FILE_LEN_ERR = -4,


	aWork_FILE_FOPEN_ERROR = -10,
	aWork_FILE_LEN_ERROR = -11,
	aWork_FILE_READ_ERROR = -12,
};


gint oper_file(const gchar *pPath, const gchar *pOper, const gchar *pData, const guint nDataLen);
gint del_file(const gchar *pPath);
gint read_file(const gchar *pPath, gchar **pData);

gint read_file_no_malloc(const gchar *pPathName, gchar *pReadBuf, gint nLen);
gint load_string(gchar *pData, gint nMaxLen, const gchar *pFile);


#endif
