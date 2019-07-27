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

#endif
