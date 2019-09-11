#ifndef MEDIA_SERVER_H
#define MEDIA_SERVER_H

#include <libsoup/soup.h>

enum {
	eMEDIA_SERVER_SUCCESS	= 0,
	eMEDIA_SERVER_FAILE		= -1,
	eMEDIA_SERVER_INVALID	= -2,

	eMSG_GET_ERR 			= -200,
	eMSG_JSON_PARSE_ERR 	= -201,
	eMSG_JSON_LOAD_ERR 		= -202,
	eMSG_JSON_GET_ROOT_ERR 	= -203,
	eMSG_JSON_GET_TYPE_ERR 	= -204,
	eMSG_JSON_TYPE_VALUE_ERR= -205,
	eMSG_JSON_TYPE_NOT_FIND	= -206,

	eMSG_JSON_PLAY_FIND_MEMBER_ERR 	= -220,
	eMSG_JSON_PLAY_GET_MEMBER_ERR 	= -221,
	eMSG_JSON_PLAY_KEY_MALLOC_ERR 	= -222,
	eMSG_JSON_PLAY_VIDEO_MALLOC_ERR = -223,
	eMSG_JSON_PLAY_NAME_MALLOC_ERR 	= -224,
	eMSG_JSON_PLAY_URL_MALLOC_ERR 	= -225,
	eMSG_JSON_PLAY_HASH_INSTALL_ERR = -226,
	eMSG_JSON_PLAY_START_VIDEO_ERR 	= -227,
	eMSG_JSON_PLAY_KEY_ALREADY_EXIST= -228,

	eMSG_JSON_STOP_FIND_MEMBER_ERR 	= -230,
	eMSG_JSON_STOP_GET_MEMBER_ERR 	= -231,
	eMSG_JSON_STOP_HASH_REMOVE_ERR 	= -232,
	eMSG_JSON_STOP_HASH_FIND_ERR 	= -233,
	eMSG_JSON_STOP_HASH_VLAUE_ERR 	= -234,

	eMSG_JSON_REPLY_BUILD_ERR 		= -240,
	eMSG_JSON_GENERATOR_NEW_ERR 	= -241,
	eMSG_JSON_BUILD_GET_ROOT_ERR 	= -242,
	eMSG_JSON_GENERATOR_TO_DATA_ERR = -243,

	eMSG_RTCP_FILE_INIT_ERR				= -300,
	eMSG_RTCP_FILE_GET_PEOCESS_NAME_ERR	= -301,
	eMSG_RTCP_FILE_GET_TIME_ERR			= -302,
	eMSG_RTCP_FILE_WRITE_LIST_ERR		= -303,
	eMSG_RTCP_FILE_PARSE_LIST_ERR		= -304,
	eMSG_RTCP_FILE_PARSE_LOAD_ERR		= -305,
	eMSG_RTCP_FILE_GET_DATA_ERR			= -307,
	eMSG_RTCP_FILE_GET_ROOT_ERR 		= -306,
	eMSG_RTCP_FILE_GET_ARRAY_ERR 		= -307,
	eMSG_RTCP_FILE_STRING_TIME_ERR 		= -308,
	eMSG_RTCP_FILE_GET_GEN_ERR 			= -309,
	eMSG_RTCP_FILE_GET_FILE_NAME_ERR 	= -310,

	eMSG_JSON_WRIEW_STREAM_NEW_ERR		= -350,
};


#define CONFIG_FILE 		"./config.json"
#define CONFIG_GET_PATH 	"./api/video/config"

typedef struct {
  GMainLoop *loop;
  volatile gboolean g_running_flag;
  // oupSession *session;
  // SoupMessage *msg;
  // SoupWebsocketConnection *client;
  // GError *client_error;
  SoupServer *soup_server;
  SoupWebsocketConnection *server;
  // gboolean no_server;
  // GIOStream *raw_server;
} AppContext;


SoupServer* start_server(AppOption *opt, AppContext *app);
void stop_server(SoupServer* server);
#endif
