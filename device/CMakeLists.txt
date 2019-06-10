project(gst-webrtc-device C)

set(CMAKE_BUILD_TYPE DEBUG)
# DEBUG/MINSIZEREL/RELWITHDEBINFO/RELEASE
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fvisibility=hidden -fvisibility-inlines-hidden -Werror -Wall -Wextra -Wno-unused-parameter -Wno-ignored-qualifiers
#                                           -Werror=missing-field-initializers")

find_package(glib-2.0 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)
#pkg_check_modules(SOUP_GLIB REQUIRED libsoup)

add_executable(gst-webrtc-device "")
target_sources(gst-webrtc-device
    PRIVATE
        webrtc-gstreamer-sender.c
)

target_include_directories(${PROJECT_NAME} PRIVATE ${JSON_GLIB_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    gst-webrtc-lib
    glib-2.0
    gobject-2.0
    ${JSON_GLIB_LIBRARIES}
)
