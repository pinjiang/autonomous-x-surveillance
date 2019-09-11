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
JsonNode *get_root_node_from_file(JsonParser **parser, const gchar *file)
{
	GError *error = NULL;
	(*parser) = json_parser_new ();
	if (NULL == (*parser)) {
		return NULL;
	}
	if (FALSE == json_parser_load_from_file ((*parser), file, &error)) {
		g_error_free(error);
		g_object_unref(*parser);
		return NULL;
	}
	JsonNode *root = json_parser_get_root (*parser);
	if (NULL == root || !JSON_NODE_HOLDS_OBJECT(root)) {
		g_object_unref(*parser);
		return NULL;
	}
	return root;
}
/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
JsonArray *get_array_from_node(JsonNode *root, gchar *name)
{
	if (NULL == root) {
		return NULL;
	}
	JsonObject *object = json_node_get_object (root);
	if (NULL == object || !json_object_has_member (object, name)) {
		return NULL;
	}
	JsonArray *files = json_object_get_array_member(object, name);
	if (NULL == files) {
		return NULL;
	}
	return files;
}
/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gint write_json_to_file(const gchar *file, JsonNode *node)
{
	g_assert(NULL != file && NULL != node);

	gsize str_len = 0;
	JsonGenerator *gen = json_generator_new();
	if (NULL == gen) {
		return -1;
	}
	json_generator_set_root(gen, node);

	gchar *str = json_generator_to_data(gen, &str_len);
	if (aWork_FILE_SUCCESS != oper_file(file, "w+", str, str_len)) {
		g_free(str);
		g_object_unref (gen);
		return -2;
	}
	g_free(str);
	g_object_unref (gen);
	return 0;
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

/*********************************************************************************************
 * Description:获取软件的进程名称                                                               *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
gint get_process_name(GString *name)
{
	if (NULL == name) {
		return -1;
	}
#if 0
	gchar app_name[256] = {0};
	//需要proc文件系统支持
	gint count = readlink("/proc/self/exe", app_name, 1024);
	if (count < 0 || count > 256) {
		return -2;
	}
	app_name[count] = '\0';

	gchar *file_name = g_path_get_basename(app_name);
	if (NULL == file_name) {
		return -3;
	}
	name = g_string_append (name, file_name);
	if (NULL == name) {
		g_free(file_name);
		return -4;
	}
	g_free(file_name);
#else
	name = g_string_append (name, "GStreamer");
	if (NULL == name) {
		return -4;
	}
#endif
	return 0;
}


/********************************************************************************************************************
@功  能: 获取文件的所有内容
@入  口：pPath: 目录路径
		pData: 文件内容
@返  回：aWork_FILE_SUCCESS: 成功		小于0:错误
		 只有返回成功后，才需要释放资源
********************************************************************************************************************/
gint read_file(const gchar *pPath, gchar **pData)
{
	if (NULL == pPath) { return aWork_FILE_INVALID; }

	/* 1.打开文件 */
	FILE *f = fopen(pPath, "rb");
	if (NULL == f) {
		return aWork_FILE_OPEN_FILE_ERR;
	}

	/* 2.申请空间 */
	fseek(f, 0, SEEK_END);
	gint64 lLen = ftell(f);
	fseek(f, 0, SEEK_SET);
	(*pData) = (gchar*)malloc(lLen+1);
	memset((*pData), 0, lLen+1);

	/* 3.读文件 */
	if ((guint64)lLen != fread((*pData), 1, lLen, f)) {
		free((*pData));
		(*pData) = NULL;
		return aWork_FILE_READ_FILE_LEN_ERR;
	}
	if (NULL != f) {
		fclose(f);
	}
	return aWork_FILE_SUCCESS;
}

/********************************************************************************************************************
@功  能: 对文件操作
@入  口：pPath: 目录路径
		pOper: 文件写入字符串的选项
		pData: 写入的字符串
		nDataLen: 字符串长度
@返  回：aWork_FILE_SUCCESS: 成功		小于0:错误
********************************************************************************************************************/
gint oper_file(const gchar *pPath, const gchar *pOper, const gchar *pData, const guint nDataLen)
{
	if (NULL == pPath || NULL == pOper || NULL == pData || nDataLen == 0) {
		return aWork_FILE_INVALID;
	}

	/* 1.按照选项的方式打开文件 */
	FILE *f = fopen(pPath, pOper);
	if (NULL == f) {
		return aWork_FILE_OPEN_FILE_ERR;
	}
	/* 2.写文件 */
	if (nDataLen != fwrite(pData, 1, nDataLen, f)) {
		return aWork_FILE_READ_FILE_LEN_ERR;
	}
	fflush(f);
	fclose(f);
	return aWork_FILE_SUCCESS;
}


/********************************************************************************************************************
@功  能: 删除文件
@入  口：pPath: 目录路径
@返  回：aWork_FILE_SUCCESS: 成功		小于0:错误
********************************************************************************************************************/
gint del_file(const gchar *pPath)
{
	if (NULL == pPath) { return aWork_FILE_INVALID; }

	if (0 != remove(pPath)) {
		return aWork_FILE_FAILE;
	}
	return aWork_FILE_SUCCESS;
}


/********************************************************************************************************************
@功  能: 读文件（不带申请空间）
@入  口：pPathName: 文件路径加名称
		pReadBuf: 数据缓存区
		nLen: 最大长度
@返  回：aWork_FILE_SUCCESS: 成功		小于0:错误
********************************************************************************************************************/
gint read_file_no_malloc(const gchar *pPathName, gchar *pReadBuf, gint nLen)
{
	if (NULL == pPathName || NULL == pReadBuf || 0 == nLen) {
		return aWork_FILE_INVALID;
	}

	gint ret = aWork_FILE_FAILE;
	FILE *f = fopen(pPathName, "rb");
	if (f == NULL) {
		return aWork_FILE_FOPEN_ERROR;
	}
	fseek(f, 0, SEEK_END);
	gint64 len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (nLen  < len) {
		return aWork_FILE_LEN_ERROR;
	}
	ret = fread(pReadBuf, 1, len, f);
	if (0 == ret) {
		return aWork_FILE_READ_ERROR;
	}

	if (NULL != f) {
		fclose(f);
	}

	return ret;
}

/********************************************************************************************************************
@功  能: 加载string文件（不带申请空间）
@入  口：pData: 存放数据缓存
		nMaxLen: 存放最大长度
		pFile: 目录路径
@返  回：aWork_FILE_SUCCESS: 成功		小于0:错误
********************************************************************************************************************/
gint load_string(gchar *pData, gint nMaxLen, const gchar *pFile)
{
	if (NULL == pData || NULL == pFile) {
		return aWork_FILE_INVALID;
	}
	gint nReadLen = 0;
	gchar szReadBuf[512] = {0};

	nReadLen = read_file_no_malloc(pFile, szReadBuf, sizeof(szReadBuf));
	if (nReadLen < 0) {
		return aWork_FILE_READ_ERROR;
	}

	memcpy(pData, szReadBuf, MIN(nReadLen, nMaxLen));

	return MIN(nReadLen, nMaxLen);
}

