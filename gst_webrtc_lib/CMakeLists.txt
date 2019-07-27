project(gst-webrtc-lib C)

set(CMAKE_BUILD_TYPE DEBUG)
# DEBUG/MINSIZEREL/RELWITHDEBINFO/RELEASE
set(CMAKE_C_FLAGS "-Werror -Wextra -DG_LOG_DOMAIN=\\\"gst_media_server\\\"")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fvisibility=hidden -fvisibility-inlines-hidden -Werror -Wall -Wextra -Wno-unused-parameter -Wno-ignored-qualifiers
#                                           -Werror=missing-field-initializers")

find_package(GStreamer 1.0 REQUIRED COMPONENTS
    app
    audio
    video
    webrtc
)

find_package(glib-2.0 REQUIRED)
find_package(LIBSOUP24 REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)
#pkg_check_modules(SOUP_GLIB REQUIRED libsoup)

add_library(gst-webrtc-lib "")

target_sources(gst-webrtc-lib
    PRIVATE
         ${CMAKE_CURRENT_LIST_DIR}/gst_webrtc.c
         ${CMAKE_CURRENT_LIST_DIR}/msg_handlers.c
         ${CMAKE_CURRENT_LIST_DIR}/common.c
         ${CMAKE_CURRENT_LIST_DIR}/../utils/utils.c
    PUBLIC
         ${CMAKE_CURRENT_LIST_DIR}/defs.h
         ${CMAKE_CURRENT_LIST_DIR}/gst_webrtc.h
         ${CMAKE_CURRENT_LIST_DIR}/msg_handlers.h
         ${CMAKE_CURRENT_LIST_DIR}/common.h
)

target_include_directories(${PROJECT_NAME} PUBLIC  ${GSTREAMER_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${GSTREAMER_APP_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC  ${LIBSOUP24_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC  ${JSON_GLIB_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC  ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC  ${CMAKE_SOURCE_DIR}/utils)

target_link_libraries(${PROJECT_NAME}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    ${GSTREAMER_AUDIO_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${GSTREAMER_SDP_LIBRARIES}
    ${GSTREAMER_WEBRTC_LIBRARIES}
    ${LIBSOUP24_LIBRARY}
    glib-2.0
    gobject-2.0
    ${JSON_GLIB_LIBRARIES}
)
