#include <stdlib.h>
#include <glib.h>
#include <gst/gst.h>
#include <gobject/gvaluetypes.h>
#include <string.h>

#include "sys.h"
#include "utils.h"
#include "media_server.h"
enum {
	eSUCCESS	= 0,
	eFAILE		= -1,
	eINVALID	= -2,

	eCMDLINE_CREATE_ERR = -10,
	eCMDLINE_PARSE_ERR = -11,
	eCMDLINE_ARGC_ERR = -12,
	eCMDLINE_TASK_NAME_ERR = -13,

	eMAIN_LOOP_CREATE_ERR = -20,
	eMAIN_START_SERVER_ERR = -21,
};
/******************************************************/
static AppContext g_app = {0};
static AppOption  g_opt = {NULL, 20000, FALSE, NULL, NULL, "test", "../line_gauge/data"};
static GOptionEntry g_entries[] = {
  { "verbose",      'v', 0, G_OPTION_ARG_STRING, &g_opt.verbose,      "Be verbose", NULL },
  { "disable-ssl",  0,   0, G_OPTION_ARG_NONE,   &g_opt.disable_ssl,  "Disable ssl", NULL },
  { "cert-file",    'c', 0, G_OPTION_ARG_STRING, &g_opt.tls_cert_file,"Use FILE as the TLS certificate file", "FILE" },
  { "key-file",     'k', 0, G_OPTION_ARG_STRING, &g_opt.tls_key_file, "Use FILE as the TLS private key file", "FILE" },
  { "port",         'p', 0, G_OPTION_ARG_INT,    &g_opt.listen_port,  "Port to listen on", NULL },
  { "task",         't', 0, G_OPTION_ARG_STRING, &g_opt.task_name,    "Task name of test", NULL },
  { "result_file_path",'f', 0, G_OPTION_ARG_STRING, &g_opt.result_file_path,"Result file path", "Path" },
  { NULL }
};

/* 内部函数 */
static gint parse_command_line(gint argc, gchar *argv[]);
static gint set_command_line_opt(const AppOption *opt);
static void quit (gint sig);

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
int main(int argc, char *argv[]) {

	gint ret = eSUCCESS;

	g_print("Version:%d.%d.%d\n", MAJOR_VERSION, MIN_VERSION, REVISION);
	/* 1.解析命令行参数，并设置 */
	ret = parse_command_line(argc, argv);
	if (eSUCCESS != ret) {
		return ret;
	}
	ret = set_command_line_opt(&g_opt);
	if (eSUCCESS != ret) {
		return ret;
	}

	/* 2.开始启动服务 */
	glib_log_info("Media Server Started");
	signal (SIGINT, quit);
#if not WIN32
	signal(SIGKILL, quit);
#endif
	signal (SIGTERM, quit);

	g_app.loop = g_main_loop_new (NULL, TRUE);
	if (NULL == g_app.loop) {
		glib_log_warning("Main loop created err !!!");
		return eMAIN_LOOP_CREATE_ERR;
	}
	/* Start Web Server for Web Page Interaction */
	g_app.soup_server = start_server(&g_opt, &g_app);
	if (NULL == g_app.soup_server) {
		glib_log_warning("Server start err");
		g_main_loop_unref (g_app.loop);
		return eMAIN_LOOP_CREATE_ERR;
	}

	glib_log_info("Media Server Running...");
	g_main_loop_run (g_app.loop);

	/* 3.服务停止，清理资源 */
	if (NULL != g_app.soup_server) {
		glib_log_info("Stop server...");
		stop_server(g_app.soup_server);
	}
	glib_log_info("Unref main loop...");
	g_main_loop_unref (g_app.loop);

	return eSUCCESS;
}


/*********************************************************************************************
 * Description:解析命令行参数                                                                  *
 *                                                                                           *
 * Input : 	gint argc:命令行个数
 * 			gchar *argv[] :命令行参数                                                         *
 * Return:  解析结果                                                                          *
 *********************************************************************************************/
static gint parse_command_line(gint argc, gchar *argv[])
{
	GError *error = NULL;
	GOptionContext *opts = g_option_context_new (NULL);
	if (NULL == opts) {
		  g_printerr ("Option context created err!!!");
		  return eCMDLINE_CREATE_ERR;
	}

	g_option_context_add_main_entries (opts, g_entries, NULL);
	g_option_context_add_group (opts, gst_init_get_option_group ());
	if (!g_option_context_parse (opts, &argc, &argv, &error)) {
		g_printerr ("Could not parse arguments: %s\n", error->message);
		g_printerr ("%s", g_option_context_get_help (opts, TRUE, NULL));
		g_free(error);
		g_option_context_free (opts);
		return eCMDLINE_PARSE_ERR;
	}

	if (argc != 1) {
	  g_printerr ("%s", g_option_context_get_help (opts, TRUE, NULL));
	  g_option_context_free (opts);
	  return eCMDLINE_ARGC_ERR;
	}

	g_option_context_free (opts);
	return eSUCCESS;
}

/*********************************************************************************************
 * Description: 设置命令行选项                                                                 *
 *                                                                                           *
 * Input : 	const AppOption *opt：选项缓冲区                                                        	 *
 * Return:  设置结果                                                                          *
 *********************************************************************************************/
static gint set_command_line_opt(const AppOption *opt)
{
	g_assert(NULL != opt);

	if (strlen(opt->task_name) >= TASK_NAME_MAX_LEN) {
		  g_printerr ("Task name is too long.");
		  return eCMDLINE_TASK_NAME_ERR;
	}
	if(!g_strcmp0("vv", opt->verbose)) {   /* If -vv passed set to ONLY debug */
		g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_INFO | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING \
					| G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_DEBUG, log_handler_cb, NULL);

	} else if(!g_strcmp0("v", opt->verbose)) {   /* If -v passed set to ONLY info */
		g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_INFO | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_MESSAGE, log_handler_cb, NULL);

	} else { /* For everything else, set to back to default*/
		g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_INFO | G_LOG_LEVEL_CRITICAL , log_handler_cb, NULL);
	}

	return eSUCCESS;
}

/*********************************************************************************************
 * Description: 退出服务                                                                      *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void quit (gint sig)
{
	/* Exit cleanly on ^C in case we're valgrinding. */
	glib_log_debug("Quit signal:%d", sig);
	//停止主循环
	if (NULL != g_app.loop) {
		if (TRUE == g_main_loop_is_running(g_app.loop)) {
			glib_log_debug("Stop main loop...");
			g_main_loop_quit(g_app.loop);
		}
	}
}



#if 0
  gst_init (NULL, NULL);
  //gtk环境初始化
  gtk_init (NULL, NULL);
  //创建一个窗口
  GtkWidget *windowHead = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  //设置窗口标题
  // gtk_window_set_title(GTK_WINDOW(windowHead), "hbox");
  gtk_window_fullscreen((GtkWindow*)windowHead);
  //创建一个垂直容器

  // GtkWidget *vbox = gtk_vbox_new(TRUE, 10);
  // 创建水平容器
  GtkWidget *hbox = gtk_hbox_new(TRUE, 10);
  // GtkWidget *hbox2 = gtk_hbox_new(TRUE, 10);
  //创建显示视频的widget
  video1 = gtk_drawing_area_new ();
  video2 = gtk_drawing_area_new ();
  video3 = gtk_drawing_area_new ();
  // video4 = gtk_drawing_area_new ();

  //在容器中添加组件
  gtk_container_add(GTK_CONTAINER(hbox), video1);
  gtk_container_add(GTK_CONTAINER(hbox), video3);
  gtk_container_add(GTK_CONTAINER(hbox), video2);
  // gtk_container_add(GTK_CONTAINER(hbox1), video4);

  // gtk_container_add(GTK_CONTAINER(vbox), hbox1);
  // gtk_container_add(GTK_CONTAINER(vbox), hbox2);

  //将垂直容器加入到窗口中
  gtk_container_add(GTK_CONTAINER(windowHead), hbox);
  // gtk_container_add (GTK_CONTAINER (windowHead), video_window);
  gtk_window_set_default_size (GTK_WINDOW (windowHead), 3840, 1080);
  gtk_widget_show_all (windowHead);
#endif

