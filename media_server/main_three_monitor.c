#include <stdlib.h>
#include <glib.h>
#include <fcgi_stdio.h>
#include <gst/gst.h>
#include <gobject/gvaluetypes.h>
#include <string.h>
#include <pthread.h>

#include <libsoup/soup.h>

//#define GST_USE_UNSTABLE_API
//#include <gst/webrtc/webrtc.h>

#include "global.h"
#include "interface.h"
#include "ullvs-httpd.h"
#include "ullvs-gst-webrtc.h"

gint port = 4000;
const char *tls_cert_file, *tls_key_file;

GMainLoop *loop;
GstElement *pipe1, *pipe2, *pipe3, *pipe4, *webrtc1, *webrtc2, *webrtc3, *webrtc4;
GtkWidget *video1, *video2, *video3, *video4;

SoupWebsocketConnection *ws_conn = NULL;
enum AppState app_state1 = 0;
enum AppState app_state2 = 0;
enum AppState app_state3 = 0;
enum AppState app_state4 = 0;
// const gchar *server_url = "ws://122.112.211.178:8443";
gboolean disable_ssl = FALSE;
gint values = 0;
Context g_ctx; 

static GOptionEntry entries[] = {
  // { "server", 0, 0, G_OPTION_ARG_STRING, &server_url, "Signalling server to connect to", "URL" },
  { "disable-ssl", 0, 0, G_OPTION_ARG_NONE, &disable_ssl, "Disable ssl", NULL },
  { "cert-file", 'c', 0, G_OPTION_ARG_STRING, &tls_cert_file, "Use FILE as the TLS certificate file", "FILE" },
  { "key-file", 'k', 0,  G_OPTION_ARG_STRING, &tls_key_file,  "Use FILE as the TLS private key file", "FILE" },
  { "port", 'p', 0,      G_OPTION_ARG_INT, &port, "Port to listen on", NULL },
  { NULL }
};

static void quit (int sig) {
	/* Exit cleanly on ^C in case we're valgrinding. */
	exit (0);
}


int main(int argc, char *argv[]) {

 	GOptionContext *opts;
	GMainLoop *loop;
	SoupServer *server;
	GSList *uris, *u;
	char *str;
	GTlsCertificate *cert;
	GError *error = NULL;

	opts = g_option_context_new (NULL);
	g_option_context_add_main_entries (opts, entries, NULL);
	g_option_context_add_group (opts, gst_init_get_option_group ());
	if (!g_option_context_parse (opts, &argc, &argv, &error)) {
		g_printerr ("Could not parse arguments: %s\n", error->message);
		g_printerr ("%s", g_option_context_get_help (opts, TRUE, NULL));
		exit (1);
	}
	if (argc != 1) {
		g_printerr ("%s",
			    g_option_context_get_help (opts, TRUE, NULL));
		exit (1);
	}
	g_option_context_free (opts);
	signal (SIGINT, quit);
	if (tls_cert_file && tls_key_file) {
		cert = g_tls_certificate_new_from_files (tls_cert_file, tls_key_file, &error);
		if (error) {
			g_printerr ("Unable to create server: %s\n", error->message);
			exit (1);
		}
		server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "simple-httpd ",
					  SOUP_SERVER_TLS_CERTIFICATE, cert,
					  NULL);
		g_object_unref (cert);

		soup_server_listen_all (server, port, SOUP_SERVER_LISTEN_HTTPS, &error);
	} else {
		server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "simple-httpd ",
					  NULL);
		soup_server_listen_all (server, port, 0, &error);
	}
	soup_server_add_handler (server, NULL, server_callback, NULL, NULL);
	uris = soup_server_get_uris (server);
	for (u = uris; u; u = u->next) {
		str = soup_uri_to_string (u->data, FALSE);
		g_print ("Listening on %s\n", str);
		g_free (str);
		soup_uri_free (u->data);
	}
	g_slist_free (uris);
	g_print ("\nWaiting for requests...\n");    
    gst_init (NULL, NULL);
    //gtk环境初始化
    gtk_init (NULL, NULL);
	//创建一个窗口
	GtkWidget *windowHead = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//设置窗口标题
	// gtk_window_set_title(GTK_WINDOW(windowHead), "hbox");
	gtk_window_fullscreen((GtkWindow*)windowHead);
	//创建一个垂直容器
	GtkWidget *vbox = gtk_vbox_new(TRUE, 10);
	//创建水平容器
	GtkWidget *hbox1 = gtk_hbox_new(TRUE, 10);
	GtkWidget *hbox2 = gtk_hbox_new(TRUE, 10);
	//创建显示视频的widget
	video1 = gtk_drawing_area_new ();
	video2 = gtk_drawing_area_new ();
	video3 = gtk_drawing_area_new ();
        video4 = gtk_drawing_area_new ();
	//在容器中添加组件
	gtk_container_add(GTK_CONTAINER(hbox2), video2);
	gtk_container_add(GTK_CONTAINER(hbox2), video3);
	gtk_container_add(GTK_CONTAINER(hbox1), video1);
	gtk_container_add(GTK_CONTAINER(hbox1), video4);
	gtk_container_add(GTK_CONTAINER(vbox), hbox1);
	gtk_container_add(GTK_CONTAINER(vbox), hbox2);
	//将垂直容器加入到窗口中
	gtk_container_add(GTK_CONTAINER(windowHead), vbox);
	// gtk_container_add (GTK_CONTAINER (windowHead), video_window);
	gtk_window_set_default_size (GTK_WINDOW (windowHead), 500, 500);
	gtk_widget_show_all (windowHead);
	loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (loop);
	g_main_loop_unref (loop);

	return 0;
}
