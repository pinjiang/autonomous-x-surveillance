#include "utils.h"

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
int compare_strings (gconstpointer a, gconstpointer b) {
  const char **sa = (const char **)a;
  const char **sb = (const char **)b;

  return strcmp (*sa, *sb);
}


/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gchar* get_string_from_json_object (JsonObject * object) {
  JsonNode *root;
  JsonGenerator *generator;
  gchar *text;

  /* Make it the root node */
  root = json_node_init_object (json_node_alloc (), object);
  generator = json_generator_new ();
  json_generator_set_root (generator, root);
  text = json_generator_to_data (generator, NULL);

  /* Release everything */
  g_object_unref (generator);
  json_node_free (root);
  return text;
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
JsonObject* parse_json_object(const gchar* text) {
  JsonNode *root;
  JsonObject *object, *child;
  JsonParser *parser = json_parser_new ();
  if (!json_parser_load_from_data (parser, text, -1, NULL)) {
    g_printerr ("Unknown message '%s', ignoring", text);
    g_object_unref (parser);
    return NULL;
  }

  root = json_parser_get_root (parser);
  if (!JSON_NODE_HOLDS_OBJECT (root)) {
    g_printerr ("Unknown json message '%s', ignoring", text);
    g_object_unref (parser);
    return NULL;
  }

  object = json_node_get_object (root);
  return object;          
}

/*********************************************************************************************
 * Description:                                                                              *                                                 
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
guint hash_func(gconstpointer key) {
  return g_str_hash(key);
}


/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gboolean key_equal_func(gconstpointer a, gconstpointer b) {
  return g_str_equal(a, b);
}


/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static const gchar * log_level_to_string (GLogLevelFlags level)
{
  switch (level)
    {
      case G_LOG_LEVEL_ERROR:    return "ERROR";
      case G_LOG_LEVEL_CRITICAL: return "CRITICAL";
      case G_LOG_LEVEL_WARNING:  return "WARNING";
      case G_LOG_LEVEL_MESSAGE:  return "MESSAGE";
      case G_LOG_LEVEL_INFO:     return "INFO";
      case G_LOG_LEVEL_DEBUG:    return "DEBUG";
      default: return "UNKNOWN";
    }
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
void log_handler_cb (const gchar *log_domain,
                GLogLevelFlags    log_level,
                const gchar      *message,
                gpointer        user_data)
{
  const gchar *log_level_str;

  log_level_str = log_level_to_string (log_level & G_LOG_LEVEL_MASK);

  /* Use g_printerr() for warnings and g_print() otherwise. */
  if (log_level <= G_LOG_LEVEL_WARNING) {
    g_printerr ("%s: %s: %s\n", log_domain, log_level_str, message);
  }
  else {
    g_print ("%s: %s: %s\n", log_domain, log_level_str, message);
  } 
}

