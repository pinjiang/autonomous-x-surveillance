CC	:= gcc
LIBS	:= $(shell pkg-config --libs --cflags gobject-2.0 gstreamer-webrtc-1.0 gstreamer-sdp-1.0 libsoup-2.4 json-glib-1.0)
CFLAGS	:= -O0 -ggdb -Wall -fno-omit-frame-pointer

webrtc-gstreamer-sender: webrtc-gstreamer-sender.c common.c msg_handlers.c gst_webrtc.c
	"$(CC)" $(CFLAGS) $^ $(LIBS) -o $@
