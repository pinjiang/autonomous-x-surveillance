#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

gchar* get_string_from_json_object (JsonObject * object);

JsonObject* parse_json_object(const gchar* text);

guint hash_func(gconstpointer key);

gboolean key_equal_func(gconstpointer a, gconstpointer b);

void log_handler_cb (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);



/// 日志输出头.线程号(任务号) + [时间]
#define CLOG_PREFIX "--%s==%s==[%d]: "

///日志结束
#if 0
	#define CLOG_END	"\r\n"
#else
	#define	CLOG_END
#endif
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
