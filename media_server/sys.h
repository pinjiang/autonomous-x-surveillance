#ifndef SYS_H
#define SYS_H

#include <stdlib.h>
#include <glib.h>
#include <gst/gst.h>
#include <gobject/gvaluetypes.h>
#include <string.h>
/******************************************************/
#define	MAJOR_VERSION	1
#define MIN_VERSION		0
#define REVISION		1
/******************************************************/
/* 命令行参数缓冲区 */
#define TASK_NAME_MAX_LEN	64
typedef struct {
  const gchar *verbose;
  guint  listen_port;
  gboolean disable_ssl;
  const gchar *tls_cert_file;
  const gchar *tls_key_file;
  const gchar *task_name;
  const gchar *result_file_path;	//检测结果存储路径
} AppOption;

#endif
