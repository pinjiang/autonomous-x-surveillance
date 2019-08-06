#include <stdlib.h>
#include <glib.h>
#include <gst/gst.h>
#include <gobject/gvaluetypes.h>
#include <string.h>
#include <pthread.h>

#include "utils.h"
#include "media_server.h"

// GMainLoop *loop;
// GtkWidget *video1, *video2, *video3, *video4;
// SoupWebsocketConnection *ws_conn = NULL;

typedef struct {
  const gchar *verbose;  
  gint  listen_port;
  gboolean disable_ssl;
  const char *tls_cert_file;
  const char *tls_key_file;
} AppOption;

static AppOption  g_opt = {NULL, 20000, FALSE, NULL, NULL};

static GOptionEntry entries[] = {
  { "verbose",      'v', 0, G_OPTION_ARG_STRING, &g_opt.verbose,      "Be verbose", NULL },
  { "disable-ssl",  0,   0, G_OPTION_ARG_NONE,   &g_opt.disable_ssl,  "Disable ssl", NULL },
  { "cert-file",    'c', 0, G_OPTION_ARG_STRING, &g_opt.tls_cert_file,"Use FILE as the TLS certificate file", "FILE" },
  { "key-file",     'k', 0, G_OPTION_ARG_STRING, &g_opt.tls_key_file, "Use FILE as the TLS private key file", "FILE" },
  { "port",         'p', 0, G_OPTION_ARG_INT,    &g_opt.listen_port,  "Port to listen on", NULL },
  { NULL }
};

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
static void quit (int sig) {
  /* Exit cleanly on ^C in case we're valgrinding. */
  exit (0);
}

/*********************************************************************************************
 * Description:                                                                              *
 *                                                                                           *
 * Input :                                                                                   *
 * Return:                                                                                   *
 *********************************************************************************************/
int main(int argc, char *argv[]) {

  GOptionContext *opts;
  GError *error = NULL;
  AppContext app;

  opts = g_option_context_new (NULL);
  g_option_context_add_main_entries (opts, entries, NULL);
  g_option_context_add_group (opts, gst_init_get_option_group ());
  if (!g_option_context_parse (opts, &argc, &argv, &error)) {
    g_printerr ("Could not parse arguments: %s\n", error->message);
    g_printerr ("%s", g_option_context_get_help (opts, TRUE, NULL));
    exit (1);
  }
  if (argc != 1) {
    g_printerr ("%s", g_option_context_get_help (opts, TRUE, NULL));
    exit (1);
  }
  g_option_context_free (opts);

  if(!g_strcmp0("vv", g_opt.verbose)) {   /* If -vv passed set to ONLY debug */
    g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_INFO | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING \
                      | G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_DEBUG, log_handler_cb, NULL);

  } else if(!g_strcmp0("v", g_opt.verbose)) {   /* If -v passed set to ONLY info */
    g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_INFO | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_MESSAGE, log_handler_cb, NULL);

  } else { /* For everything else, set to back to default*/
    g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_INFO | G_LOG_LEVEL_CRITICAL , log_handler_cb, NULL);
  }

  /* if (!g_config_file ) {
    g_printerr ("--config is a required argument\n");
    g_printerr ("%s", g_option_context_get_help (context, TRUE, NULL));
    return -1;
  } */

  g_info("Media Server Started");
  
  signal (SIGINT, quit);
  app.loop = g_main_loop_new (NULL, TRUE);

  start_server(g_opt.listen_port, g_opt.tls_cert_file, g_opt.tls_key_file, &app);  /* Start Web Server for Web Page Interaction */
  g_main_loop_run (app.loop);

  /* Out of the main loop, clean up nicely */
  g_info("Stopping Media Server\n");

  gst_element_set_state(GST_ELEMENT(app.loop), GST_STATE_NULL);
  g_main_loop_unref (app.loop);

  return 0;
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

